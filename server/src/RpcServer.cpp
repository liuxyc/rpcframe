/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include "RpcServer.h"
#include "RpcConnection.h"
#include <thread>
#include <sys/epoll.h>  
#include <sys/socket.h>  
#include <sys/eventfd.h>
#include <netinet/in.h>  
#include <fcntl.h>  
#include <arpa/inet.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>

#define _MAX_SOCKFD_COUNT 65535 

namespace rpcframe
{

RpcServerConfig::RpcServerConfig(std::pair<const char *, int> &endpoint)
: m_thread_num(std::thread::hardware_concurrency())
, m_hostname(endpoint.first)
, m_port(endpoint.second)
{
    
    
}

RpcServerConfig::~RpcServerConfig()
{
   
}

void RpcServerConfig::setThreadNum(uint32_t thread_num)
{
    if (thread_num > 0) {
        m_thread_num = thread_num;
    }
}

uint32_t RpcServerConfig::getThreadNum()
{
    return m_thread_num;
}


RpcServer::RpcServer(rpcframe::RpcServerConfig &cfg)
: m_cfg(cfg)
, m_seqid(0)
{
    for(uint32_t i = 0; i < m_cfg.getThreadNum(); ++i) {
        RpcWorker *rw = new RpcWorker(&m_request_q, this);
        std::thread *th = new std::thread(&RpcWorker::run, rw);
        m_thread_vec.push_back(th);
        m_worker_vec.push_back(rw);
    }
    /*
    m_resp = new RpcResponser(&m_response_q, this);
    m_resp_th = new std::thread(&RpcResponser::run, m_resp);
    */
}

RpcServer::~RpcServer() {
    for(auto rw: m_worker_vec) {
        rw->stop();
    }
    for (auto th: m_thread_vec) {
        th->join();
    }
    //m_resp->stop();
    //m_resp_th->join();

}

bool RpcServer::addService(const std::string &name, rpcframe::IService *p_service)
{
    if (m_service_map.find(name) != m_service_map.end()) {
        return false;
    }
    m_service_map[name] = p_service;
    return true;
    
}

rpcframe::IService *RpcServer::getService(const std::string &name)
{
    return m_service_map[name];
}

bool RpcServer::start() {

    int epoll_fd = epoll_create(_MAX_SOCKFD_COUNT);  
    //设置非阻塞模式  
    int opts = O_NONBLOCK;  
    if(fcntl(epoll_fd,F_SETFL,opts)<0)  
    {  
        printf("set epool fd nonblock fail\n");  
        return false;  
    }  

    int listen_socket = socket(AF_INET,SOCK_STREAM,0);  
    if ( 0 > listen_socket )  
    {  
        printf("socket error!\n");  
        return false;  
    }  
    
    sockaddr_in listen_addr;  
    listen_addr.sin_family = AF_INET;  
    listen_addr.sin_port = htons(m_cfg.m_port);  
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
    listen_addr.sin_addr.s_addr = inet_addr(m_cfg.m_hostname.c_str());  
    
    int ireuseadd_on = 1;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &ireuseadd_on, sizeof(ireuseadd_on) );  
    
    if (bind(listen_socket, (sockaddr *) &listen_addr, sizeof (listen_addr) ) != 0 )  
    {  
        printf("bind error\n");  
        return false;  
    }  
    
    if (listen(listen_socket, 20) < 0 )  
    {  
        printf("listen error!\n");  
        return false;  
    }  
    else {  
        printf("Listening......\n");  
    }  

    //listen socket epoll event
    struct epoll_event ev;  
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;  
    ev.data.fd = listen_socket;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_socket, &ev);  

    m_resp_ev_fd = eventfd(0, 0);  
    if (m_resp_ev_fd == -1)
        printf("create event fd fail\n");  
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;  
    ev.data.fd = m_resp_ev_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, m_resp_ev_fd, &ev);  

    while(1) {
        struct epoll_event events[_MAX_SOCKFD_COUNT];  
        int nfds = epoll_wait(epoll_fd, events, _MAX_SOCKFD_COUNT, 2000);  
        for (int i = 0; i < nfds; i++)  
        {  
            int client_socket = events[i].data.fd;  
            if (events[i].events & EPOLLIN)
            {  
                if (client_socket == m_resp_ev_fd) {
                    ssize_t s = -1;
                    uint64_t resp_fd = -1;
                    s = read(m_resp_ev_fd, &resp_fd, sizeof(uint64_t));
                    if (s != sizeof(uint64_t))
                        printf("read resp event fail\n");

                    int sent_ret = m_conn_map[resp_fd]->sendResponse();
                    if (sent_ret == -1 ) {
                        removeConnection(client_socket);
                    }
                    else if ( sent_ret == -2 ){
                        //printf("sent partial to %d\n", client_socket);
                    }
                    else {
                        //printf("sent to %d\n", client_socket);
                    }
                    continue;

                }
                if (client_socket == listen_socket) {
                    //listen socket event
                    sockaddr_in remote_addr;  
                    int len = sizeof(remote_addr);  
                    int new_client_socket = accept(listen_socket, (sockaddr *)&remote_addr, (socklen_t*)&len );  
                    if ( new_client_socket < 0 ) {  
                        printf("accept fail %s, new_client_socket: %d\n", strerror(errno), new_client_socket);  
                    }  
                    else {
                        //NOTICE:do not use O_NONBLOCK, because we assume the first recv of pkglen 
                        // must have 4 bytes at least
                        //fcntl(new_client_socket, F_SETFL, fcntl(new_client_socket, F_GETFL) | O_NONBLOCK);
                        struct epoll_event ev;  
                        memset(&ev, 0, sizeof(ev));
                        ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
                        ev.data.fd = new_client_socket;
                        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_client_socket, &ev);  
                        m_seqid++;
                        addConnection(new_client_socket, new rpcframe::RpcConnection(new_client_socket, m_seqid));
                        //printf("new_client_socket: %d\n", new_client_socket);  

                    }
                    continue;
                }

                pkg_ret_t pkgret = getConnection(client_socket)->getRequest();
                if( pkgret.first < 0 )  
                {  
                    printf("rpc server socket disconnected: %d\n", client_socket);  
                    struct epoll_event event_del;  
                    event_del.data.fd = events[i].data.fd;  
                    event_del.events = 0;  
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_del.data.fd, &event_del);
                    removeConnection(client_socket);
                }  
                else 
                {  
                    if (pkgret.second != NULL) {
                        //got a full request, put to worker queue
                        m_request_q.push(pkgret.second);
                    }
                }  
            }  
            else  
            {  
                printf("EPOLL ERROR\n");
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);  
            }  
        }  
    }

}

bool RpcServer::hasConnection(int fd)
{
    std::lock_guard<std::mutex> mlock(m_mutex);
    return (m_conn_map.find(fd) != m_conn_map.end());
}

void RpcServer::removeConnection(int fd)
{
    std::lock_guard<std::mutex> mlock(m_mutex);
    m_conn_set.erase(m_conn_map[fd]->m_seqid);
    delete m_conn_map[fd];
    m_conn_map.erase(fd);
}

void RpcServer::addConnection(int fd, RpcConnection *conn)
{
    std::lock_guard<std::mutex> mlock(m_mutex);
    m_conn_set.emplace(conn->m_seqid, conn);
    m_conn_map.emplace(fd, conn);
}

RpcConnection *RpcServer::getConnection(int fd)
{
    std::lock_guard<std::mutex> mlock(m_mutex);
    return m_conn_map[fd];
}

void RpcServer::pushResp(std::string seqid, rpcframe::response_pkg *resp_pkg)
{
    std::lock_guard<std::mutex> mlock(m_mutex);
    if (m_conn_set.find(seqid) != m_conn_set.end()) {
        RpcConnection *conn = m_conn_set[seqid];
        conn->m_response_q.push(resp_pkg);
        uint64_t resp_fd = conn->getFd();
        ssize_t s = -1;
        s = write(m_resp_ev_fd, &(resp_fd), sizeof(uint64_t));
        if (s != sizeof(uint64_t))
            printf("write resp event fd fail\n");
    }
    else {
        printf("connection %s gone, drop resp\n", seqid.c_str());
        delete resp_pkg->data;
        delete resp_pkg;
    }
    return;
}

void RpcServer::stop() {
    for(uint32_t i = 0; i < m_cfg.getThreadNum(); ++i) {
        m_thread_vec[i]->join();
    }
    m_resp_th->join();
}

};
