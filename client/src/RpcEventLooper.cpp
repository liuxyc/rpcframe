/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include <arpa/inet.h>  
#include <fcntl.h>  
#include <netdb.h>
#include <netinet/tcp.h>  
#include <sstream>
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/epoll.h>  
#include <sys/socket.h>  
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <sys/prctl.h>

#include "RpcEventLooper.h"
#include "RpcClientConn.h"
#include "RpcClientWorker.h"
#include "RpcClient.h"
#include "util.h"
#include "rpc.pb.h"


namespace rpcframe
{

#define _MAX_SOCKFD_COUNT 65536

RpcEventLooper::RpcEventLooper(RpcClient *client, int thread_num)
: m_client(client)
, m_stop(false)
, m_fd(-1)
, m_conn(NULL)
, m_req_seqid(0)
, MAX_REQ_LIMIT_BYTE(100 * 1024 * 1024)
, m_thread_num(thread_num)
{
    if(!getHostIp(m_host_ip)) {
        printf("[ERROR]get hostip fail!");
    }
    m_epoll_fd = epoll_create(_MAX_SOCKFD_COUNT);  
    //set noblock
    int opts = O_NONBLOCK;  
    if(fcntl(m_epoll_fd,F_SETFL,opts)<0)  
    {  
        printf("set epoll fd noblock fail");
    }  

    for(int i = 0; i < m_thread_num; ++i) {
        RpcClientWorker *worker = new RpcClientWorker(this);
        std::thread *worker_th = new std::thread(&RpcClientWorker::run, worker);
        m_worker_vec.push_back(worker);
        m_thread_vec.push_back(worker_th);
    }

}

RpcEventLooper::~RpcEventLooper() {
    printf("~RpcEventLooper()\n");

}

void RpcEventLooper::stop() {
    m_stop = true;
    for(int i = 0; i < m_thread_num; ++i) {
        m_worker_vec[i]->stop();
        m_thread_vec[i]->join();
        delete m_worker_vec[i];
        delete m_thread_vec[i];
    }
}

void RpcEventLooper::removeConnection() {
    std::lock_guard<std::mutex> mlock(m_mutex);
    delete m_conn;
    m_conn = NULL;
    for (auto cb = m_cb_map.begin(); cb != m_cb_map.end(); ) {
        if (cb->second != NULL) {
            cb->second->callback(RpcStatus::RPC_DISCONNECTED, std::string(""));
        }
        m_cb_map.erase(cb++);
    }
    m_cb_timer_map.clear();
}


void RpcEventLooper::addConnection()
{
    if (m_fd != -1) {
        m_conn = new RpcClientConn(m_fd);
        struct epoll_event ev;  
        memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;  
        ev.data.fd = m_fd;
        epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_fd, &ev);  
    }
    else {
        printf("invalid fd\n");
    }
}

RpcStatus RpcEventLooper::sendReq(
        const std::string &service_name, 
        const std::string &method_name, 
        const std::string &request_data, 
        std::shared_ptr<RpcClientCallBack> cb_obj, 
        std::string &req_id) {

    if (request_data.length() >= MAX_REQ_LIMIT_BYTE) {
        printf("send data too large %lu\n", request_data.length());
        return RpcStatus::RPC_SEND_FAIL;
    }
    m_mutex.lock();
    if (m_conn == NULL) {
        if (!connect()) {
            if (cb_obj != NULL) {
                cb_obj->callback(RpcStatus::RPC_SEND_FAIL, "");
            }
            m_mutex.unlock();
            return RpcStatus::RPC_SEND_FAIL;
        }
    }
    m_mutex.unlock();
    //gen readable request_id 
    std::stringstream ssm;
    ssm << std::to_string((int)getpid())
        << "_" << std::this_thread::get_id() 
        << "_" << m_conn->getFd() 
        << "_" << std::time(nullptr)
        << "_" << m_host_ip
        << "_" << std::to_string(m_req_seqid);
    req_id = ssm.str();
    std::time_t tm_id;
    uint32_t cb_timeout = 0;
    if (cb_obj != NULL) {
        cb_obj->setReqId(req_id);
        m_mutex.lock();
        m_cb_map.insert(std::make_pair(req_id, cb_obj));
        m_mutex.unlock();
        cb_timeout = cb_obj->getTimeout();
        if( cb_timeout > 0) {
            tm_id = std::time(nullptr) + cb_timeout;
            m_mutex.lock();
            m_cb_timer_map.insert(std::make_pair(tm_id, req_id));
            m_mutex.unlock();
        }
    }
    m_mutex.lock();
    ++m_req_seqid;
    RpcStatus send_ret = m_conn->sendReq(service_name, method_name, request_data, req_id, (cb_obj == NULL), cb_timeout);
    m_mutex.unlock();
    if (send_ret == RpcStatus::RPC_SEND_OK) {
        printf("send %s\n", req_id.c_str());
    }
    else {
        printf("send fail %s\n", req_id.c_str());
        if (cb_obj != NULL) {
            m_mutex.lock();
            if( cb_timeout > 0) {
                m_cb_timer_map.erase(tm_id);
            }
            m_cb_map.erase(req_id);
            cb_obj->callback(send_ret, "");
            m_mutex.unlock();
        }
    }
    return send_ret;

}

std::shared_ptr<RpcClientCallBack> RpcEventLooper::getCb(const std::string &req_id) {
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
    if (m_cb_map.find(req_id) != m_cb_map.end()) {
        m_cb_map.erase(req_id);
    }
}

void RpcEventLooper::dealTimeoutCb() {
    std::lock_guard<std::mutex> mlock(m_mutex);
    if (!m_cb_timer_map.empty()) {
        //search timeout cb
        for(auto cb_timer_it = m_cb_timer_map.begin(); 
                cb_timer_it != m_cb_timer_map.end();) {
            auto cur_it = cb_timer_it++;
            std::string reqid = cur_it->second;
            if (m_cb_map.find(reqid) != m_cb_map.end()) { 
                std::shared_ptr<RpcClientCallBack> cb = m_cb_map[reqid];
                if(cb != NULL) {
                    std::time_t tm = cur_it->first;
                    if(std::time(nullptr) > tm) {
                        //found a timeout cb
                        //printf("%s timeout\n", cb->getReqId().c_str());
                        cb->callback(RpcStatus::RPC_CB_TIMEOUT, "");
                        m_cb_map.erase(reqid);
                    }
                    else {
                        //got item not timeout, stop search
                        break;
                    }
                }
                else {
                    printf("WARNING found timeout NULL cb\n");
                }
            }
            m_cb_timer_map.erase(cur_it);
        }
    }
}

void RpcEventLooper::run() {
    prctl(PR_SET_NAME, "Rpclooper", 0, 0, 0); 
    struct epoll_event events[_MAX_SOCKFD_COUNT];  
    while(1) {
        if (m_stop) {
            printf("RpcEventLooper stoped\n");
            removeConnection();
            break;
        }

        int nfds = epoll_wait(m_epoll_fd, events, _MAX_SOCKFD_COUNT, 1000);  
        if (m_stop) {
            printf("RpcEventLooper stoped\n");
            removeConnection();
            break;
        }

        //deal async call timeout
        //FIXME:dealTimeoutCb() may block event loop
        dealTimeoutCb();

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

    if (noBlockConnect(m_fd, m_client->getConfig().m_hostname.c_str(), 
                       m_client->getConfig().m_port, m_client->getConfig().m_connect_timeout) == -1)
    {
        printf("Connect error.\n");
        return false;
    }

    int keepAlive = 1;   
    int keepIdle = 60;   
    int keepInterval = 5;   
    int keepCount = 3;   
    setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepAlive, sizeof(keepAlive));  
    setsockopt(m_fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));  
    setsockopt(m_fd, SOL_TCP, TCP_KEEPINTVL, (void*)&keepInterval, sizeof(keepInterval));  
    setsockopt(m_fd, SOL_TCP, TCP_KEEPCNT, (void*)&keepCount, sizeof(keepCount));  
    addConnection();
    return true;
}


int RpcEventLooper::setNoBlocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option|O_NONBLOCK;
    fcntl(fd,F_SETFL, new_option);
    return old_option;
}

int RpcEventLooper::noBlockConnect(int sockfd, const char* hostname, int port, int timeoutv) {
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    std::string hostip;
    if(!getHostIpByName(hostip, hostname)) {
        printf("gethostbyname fail\n");
    }
    inet_pton(AF_INET, hostip.c_str(), &address.sin_addr);
    address.sin_port = htons(port);
    int fdopt = setNoBlocking(sockfd);
    ret = ::connect(sockfd, (struct sockaddr*)&address, sizeof(address));
    if(ret == 0) {
        fcntl(sockfd, F_SETFL,fdopt);
        return sockfd;
    }
    else if(errno != EINPROGRESS) {//if errno not EINPROGRESS, errror
        printf("unblock connect not support\n");
        ::close(sockfd);
        return -1;
    }
    fd_set writefds;
    struct timeval timeout;//connect time out
    FD_ZERO(&writefds);
    FD_SET(sockfd, &writefds);
    timeout.tv_sec = timeoutv;
    timeout.tv_usec = 0;
    ret = ::select(sockfd+1, NULL, &writefds, NULL, &timeout);
    if(ret <= 0) { 
        printf("connect %s time out\n", hostname);
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
    if(error != 0 ) {
        printf("connect host %s error:%s\n", hostname, strerror(error));
        close(sockfd);
        return -1;
    }
    //set socket back to block
    fcntl(sockfd, F_SETFL, fdopt); 
    return sockfd;
}
};
