/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include <sys/epoll.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <fcntl.h>  
#include <arpa/inet.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>
#include <sys/epoll.h>  
#include <iostream>
#include <time.h>
#include <sys/time.h>

#include "RpcEventLooper.h"
#include "RpcClientConn.h"
#include "RpcClientWorker.h"
#include "RpcClient.h"

namespace rpcframe
{

#define _MAX_SOCKFD_COUNT 65536

RpcEventLooper::RpcEventLooper(RpcClient *client)
: m_client(client)
, m_stop(false)
, m_conn(NULL)
, MAX_REQ_LIMIT_BYTE(100 * 1024 * 1024)
{
    m_epoll_fd = epoll_create(_MAX_SOCKFD_COUNT);  
    //set noblock
    int opts = O_NONBLOCK;  
    if(fcntl(m_epoll_fd,F_SETFL,opts)<0)  
    {  
        printf("set epoll fd noblock fail");
    }  

    m_worker = new RpcClientWorker(this);
    m_worker_th = new std::thread(&RpcClientWorker::run, m_worker);

}

RpcEventLooper::~RpcEventLooper() {
    m_worker->stop();
    m_worker_th->join();
    delete m_worker;
    delete m_worker_th;

}

void RpcEventLooper::stop() {
    m_stop = true;
}

void RpcEventLooper::removeConnection() {
    std::lock_guard<std::mutex> mlock(m_mutex);
    delete m_conn;
    m_conn = NULL;
    for (auto cb = m_cb_map.begin(); cb != m_cb_map.end(); ) {
        if (cb->second != NULL) {
            cb->second->callback(RpcClientCallBack::RPC_DISCONNECTED, std::string(""));
            delete cb->second;
        }
        m_cb_map.erase(cb++);
    }
}


void RpcEventLooper::addConnection()
{
    m_conn = new RpcClientConn(m_fd);
    struct epoll_event ev;  
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;  
    ev.data.fd = m_fd;
    epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_fd, &ev);  
}

bool RpcEventLooper::sendReq(const std::string &service_name, const std::string &method_name, const std::string &request_data, RpcClientCallBack *cb_obj, std::string &req_id) {
    if (request_data.length() >= MAX_REQ_LIMIT_BYTE) {
        return false;
    }
    std::lock_guard<std::mutex> mlock(m_mutex);
    if (NULL == m_conn) {
        if (!connect()) {
            if (cb_obj != NULL) {
                cb_obj->callback(RpcClientCallBack::RPC_SEND_FAIL, "");
                delete cb_obj;
            }
            return false;
        }
    }
    req_id = m_conn->genRequestId();
    bool send_ret = m_conn->sendReq(service_name, method_name, request_data, req_id, (cb_obj == NULL));
    if (cb_obj != NULL) {
        if (send_ret) {
            m_cb_map.insert(std::make_pair(req_id, cb_obj));
        }
        else {
            cb_obj->callback(RpcClientCallBack::RPC_SEND_FAIL, "");
            delete cb_obj;
        }
    }
    return send_ret;

}

RpcClientCallBack *RpcEventLooper::getCb(const std::string &req_id) {
    std::lock_guard<std::mutex> mlock(m_mutex);
    if (m_cb_map.find(req_id) != m_cb_map.end()) {
        return m_cb_map[req_id];
    }
    else {
        return NULL;
    }
}

void RpcEventLooper::removeCb(const std::string &req_id) {
    std::lock_guard<std::mutex> mlock(m_mutex);
    delete m_cb_map[req_id];
    m_cb_map.erase(req_id);
}

void RpcEventLooper::run() {

    while(1) {
        if (m_stop) {
            removeConnection();
            break;
        }

        struct epoll_event events[_MAX_SOCKFD_COUNT];  
        int nfds = epoll_wait(m_epoll_fd, events, _MAX_SOCKFD_COUNT, 1);  
        if (m_stop) {
            removeConnection();
            break;
        }
        for (int i = 0; i < nfds; i++) 
        {  
            int client_socket = events[i].data.fd;  
            if (events[i].events & EPOLLIN)//data come in
            {  
                pkg_ret_t pkgret = m_conn->getResponse();
                if( pkgret.first < 0 )  
                {  
                    printf("rpc client socket disconnected: %d\n", client_socket);  
                    struct epoll_event event_del;  
                    event_del.data.fd = client_socket;
                    event_del.events = 0;  
                    epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, event_del.data.fd, &event_del);
                    removeConnection();
                }  
                else 
                {  
                    if (pkgret.second != NULL) {
                        //got a full response, put to worker queue
                        m_response_q.push(pkgret.second);
                    }
                }  
            }  
            else  
            {  
                printf("EPOLL ERROR\n");
                epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, client_socket, &events[i]);  
            }  
        }  
        //printf("epoll loop\n");
    }
    close(m_epoll_fd);
}

bool RpcEventLooper::connect() {
    m_fd = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(m_fd < 0)
    {
        printf("socket creation failed\n");
        return false;
    }

    if (noBlockConnect(m_fd, m_client->m_cfg.m_hostname.c_str(), m_client->m_cfg.m_port, m_client->m_connect_timeout) == -1)
    {
        //printf("Connect error.\n");
        return false;
    }
    addConnection();
    return true;
}


int RpcEventLooper::setNoBlocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option|O_NONBLOCK;
    fcntl(fd,F_SETFL, new_option);
    return old_option;
}

int RpcEventLooper::noBlockConnect(int sockfd, const char* ip, int port, int timeoutv) {
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);
    int fdopt = setNoBlocking(sockfd);
    ret = ::connect(sockfd, (struct sockaddr*)&address, sizeof(address));
    if(ret == 0) {
        printf("connect immediately\n");
        fcntl(sockfd, F_SETFL,fdopt);
        return sockfd;
    }
    else if(errno != EINPROGRESS) {//if errno not EINPROGRESS, errror
        printf("unblock connect not support\n");
        ::close(sockfd);
        return -1;
    }
    fd_set readfds;
    fd_set writefds;
    struct timeval timeout;//connect time out
    FD_ZERO(&readfds);
    FD_SET(sockfd, &writefds);
    timeout.tv_sec = timeoutv;
    timeout.tv_usec = 0;
    ret = ::select(sockfd+1, NULL, &writefds, NULL, &timeout);
    if(ret <= 0) { 
        printf("connect time out\n");
        close(sockfd);
        return -1;
    }
    if(!FD_ISSET(sockfd, &writefds)) {
        printf("no events on sockfd found\n");
        close(sockfd);
        return -1;
    }
    int error = 0;
    socklen_t length = sizeof(error);
    if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &length) < 0) {
        printf("get socket option error\n");
        close(sockfd);
        return -1;
    }
    if(error != 0) {
        //printf("connect error after select %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }
    //set socket back to block
    fcntl(sockfd, F_SETFL, fdopt); 
    return sockfd;
}
};
