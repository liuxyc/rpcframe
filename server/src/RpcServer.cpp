/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include <thread>
#include <sys/epoll.h>  
#include <sys/socket.h>  
#include <sys/eventfd.h>
#include <netinet/in.h>  
#include <netinet/tcp.h>  
#include <fcntl.h>  
#include <arpa/inet.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>

#include "RpcServer.h"
#include "RpcConnection.h"

#define _MAX_SOCKFD_COUNT 65535 

namespace rpcframe
{

RpcServerConfig::RpcServerConfig(std::pair<const char *, int> &endpoint)
: m_thread_num(std::thread::hardware_concurrency())
, m_hostname(endpoint.first)
, m_port(endpoint.second)
, m_max_conn_num(1024 * 10)
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

void RpcServerConfig::setMaxConnection(uint32_t max_conn_num)
{
    m_max_conn_num = max_conn_num;
}

RpcServer::RpcServer(RpcServerConfig &cfg)
: m_cfg(cfg)
, m_seqid(0)
, m_listen_socket(-1)
, m_resp_ev_fd(-1)
, m_stop(false)
{
    m_epoll_fd = epoll_create(_MAX_SOCKFD_COUNT);  
    if( -1 == m_epoll_fd ) {
        printf("epoll_create fail %s\n", strerror(errno));
    }
    //set nonblock
    int opts = O_NONBLOCK;  
    if(fcntl(m_epoll_fd,F_SETFL,opts)<0)  
    {  
        printf("set epool fd nonblock fail\n");  
    }  
    for(uint32_t i = 0; i < m_cfg.getThreadNum(); ++i) {
        RpcWorker *rw = new RpcWorker(&m_request_q, this);
        std::thread *th = new std::thread(&RpcWorker::run, rw);
        m_thread_vec.push_back(th);
        m_worker_vec.push_back(rw);
    }
}

RpcServer::~RpcServer() {

}

bool RpcServer::addService(const std::string &name, IService *p_service)
{
    if (m_service_map.find(name) != m_service_map.end()) {
        return false;
    }
    m_service_map[name] = p_service;
    return true;
    
}

IService *RpcServer::getService(const std::string &name)
{
    return m_service_map[name];
}


bool RpcServer::startListen() {

    m_listen_socket = socket(AF_INET,SOCK_STREAM,0);  
    if ( 0 > m_listen_socket )  
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
    setsockopt(m_listen_socket, SOL_SOCKET, SO_REUSEADDR, &ireuseadd_on, sizeof(ireuseadd_on) );  
    
    if (bind(m_listen_socket, (sockaddr *) &listen_addr, sizeof (listen_addr) ) != 0 )  
    {  
        printf("bind error\n");  
        return false;  
    }  
    
    if (listen(m_listen_socket, 20) < 0 )  
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
    ev.data.fd = m_listen_socket;
    epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_listen_socket, &ev);  

    //listen resp_ev_fd, this event fd used for response data avaliable notification
    m_resp_ev_fd = eventfd(0, EFD_SEMAPHORE);  
    if (m_resp_ev_fd == -1) {
        printf("create event fd fail\n");  
        return false;
    }

    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;  
    ev.data.fd = m_resp_ev_fd;
    epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_resp_ev_fd, &ev);  

    return true;
}

bool RpcServer::start() {
    if(!startListen()) {
        printf("start listen failed\n");
    }


    struct epoll_event events[_MAX_SOCKFD_COUNT];  
    while(!m_stop) {
        int nfds = epoll_wait(m_epoll_fd, events, _MAX_SOCKFD_COUNT, 2000);  
        for (int i = 0; i < nfds; i++)  
        {  
            int client_socket = events[i].data.fd;  

            if (events[i].events & EPOLLOUT)
            {  
                if (m_conn_map.find(client_socket) != m_conn_map.end()) {
                    RpcConnection *conn = m_conn_map[client_socket];
                    int sent_ret = conn->sendResponse();
                    if (sent_ret == -1 ) {
                        removeConnection(client_socket);
                    }
                    else if ( sent_ret == -2 ){
                        //printf("OUT sent partial to %d\n", client_socket);
                    }
                    else {
                        printf("OUT sent to %d\n", client_socket);
                        //send full resp, remove EPOLLOUT flag
                        struct epoll_event event_mod;  
                        event_mod.data.fd = client_socket;
                        event_mod.events = EPOLLIN | EPOLLERR | EPOLLHUP;
                        epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, event_mod.data.fd, &event_mod);
                    }
                }
            }

            //event fd, we have data to send
            if (events[i].events & EPOLLIN)
            {  
                if (client_socket == m_resp_ev_fd) {
                    uint64_t resp_cnt = -1;
                    ssize_t s = read(m_resp_ev_fd, &resp_cnt, sizeof(uint64_t));
                    if (s != sizeof(uint64_t)) {
                        printf("read resp event fail\n");
                        continue;
                    }

                    std::string connid;
                    //if the connection still sending data,
                    //we put the event back, wait for next chance
                    if (m_resp_conn_q.peak(connid)) {
                        if (m_conn_set.find(connid) != m_conn_set.end()) {
                            RpcConnection *conn = m_conn_set[connid];
                            if(conn->isSending()) {
                                //printf("still sending pkg\n");
                                //keep eventfd filled with the count of ready connection
                                uint64_t resp_cnt = 1;
                                ssize_t s = write(m_resp_ev_fd, &(resp_cnt), sizeof(uint64_t));
                                if (s != sizeof(uint64_t)) {
                                    printf("write resp event fd fail\n");
                                }
                                continue;
                            }
                        }
                    }

                    if (m_resp_conn_q.pop(connid, 0)) {
                        if (m_conn_set.find(connid) != m_conn_set.end()) {
                            //we have resp data to send
                            RpcConnection *conn = m_conn_set[connid];
                            printf("conn %s resp queue len %lu\n", 
                                        conn->m_seqid.c_str(), 
                                        conn->m_response_q.size());
                            int sent_ret = conn->sendResponse();
                            if (sent_ret == -1 ) {
                                removeConnection(conn->getFd());
                            }
                            else if ( sent_ret == -2 ){
                                //printf("sent partial to %d\n", conn->getFd());
                                //send not finish, set EPOLLOUT flag on this fd, 
                                //until this resp send finish
                                struct epoll_event ev;  
                                memset(&ev, 0, sizeof(ev));
                                ev.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP;
                                ev.data.fd = conn->getFd();
                                epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, conn->getFd(), &ev);  
                            }
                            else {
                                printf("sent to %d\n", conn->getFd());
                                //send full resp finish, try remove EPOLLOUT flag if it already set
                                struct epoll_event event_mod;  
                                event_mod.data.fd = conn->getFd();
                                event_mod.events = EPOLLIN | EPOLLERR | EPOLLHUP;
                                epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, event_mod.data.fd, &event_mod);
                            }
                        }
                    }
                }
                else if (client_socket == m_listen_socket) {
                    //listen socket event
                    sockaddr_in remote_addr;  
                    int len = sizeof(remote_addr);  
                    int new_client_socket = accept(m_listen_socket, 
                                                   (sockaddr *)&remote_addr, 
                                                   (socklen_t*)&len );  
                    if ( new_client_socket < 0 ) {  
                        printf("accept fail %s, new_client_socket: %d\n", 
                                strerror(errno), new_client_socket);  
                    }  
                    else {
                        if( m_conn_set.size() >= m_cfg.m_max_conn_num) {
                            printf("conn number reach limit %d, close %d\n", m_cfg.m_max_conn_num, 
                                    client_socket);  
                            ::close(client_socket);
                        }
                        setSocketKeepAlive(new_client_socket);
                        //NOTICE:do not use O_NONBLOCK, because we assume the first recv of pkglen 
                        // must have 4 bytes at least
                        //fcntl(new_client_socket, F_SETFL, fcntl(new_client_socket, F_GETFL) | O_NONBLOCK);
                        struct epoll_event ev;  
                        memset(&ev, 0, sizeof(ev));
                        ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
                        ev.data.fd = new_client_socket;
                        epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, new_client_socket, &ev);  
                        m_seqid++;
                        addConnection(new_client_socket, new RpcConnection(new_client_socket, m_seqid));
                        printf("new_client_socket: %d\n", new_client_socket);  
                    }
                }
                else {

                    //data come in
                    RpcConnection *conn = getConnection(client_socket);
                    if (conn == NULL) {
                        //printf("rpc server socket already disconnected: %d\n", client_socket);  
                    }
                    else {
                        pkg_ret_t pkgret = conn->getRequest();
                        if( pkgret.first < 0 )  
                        {  
                            printf("rpc server socket disconnected: %d\n", client_socket);  
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
                }

            }  
            else  
            {  
                printf("EPOLL ERROR\n");
                epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);  
            }  
        }  
        //printf("server eloop request queue len %lu\n", m_request_q.size());
    }
    return true;
}

void RpcServer::setSocketKeepAlive(int fd)
{
    int keepAlive = 1;   
    int keepIdle = 60;   
    int keepInterval = 5;   
    int keepCount = 3;   
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepAlive, sizeof(keepAlive));  
    setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));  
    setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void*)&keepInterval, sizeof(keepInterval));  
    setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (void*)&keepCount, sizeof(keepCount));  
}

bool RpcServer::hasConnection(int fd)
{
    std::lock_guard<std::mutex> mlock(m_mutex);
    return (m_conn_map.find(fd) != m_conn_map.end());
}

void RpcServer::removeConnection(int fd)
{
    std::lock_guard<std::mutex> mlock(m_mutex);
    struct epoll_event event_del;  
    event_del.data.fd = fd;
    event_del.events = 0;  
    epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, event_del.data.fd, &event_del);
    if (m_conn_map.find(fd) != m_conn_map.end()) {
        m_conn_set.erase(m_conn_map[fd]->m_seqid);
        delete m_conn_map[fd];
        m_conn_map.erase(fd);
    }
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
    if( m_conn_map.find(fd) != m_conn_map.end()) {
        return m_conn_map[fd];
    }
    return NULL;
}

void RpcServer::pushResp(std::string conn_id, response_pkg *resp_pkg)
{
    std::lock_guard<std::mutex> mlock(m_mutex);
    if (m_conn_set.find(conn_id) != m_conn_set.end()) {
        RpcConnection *conn = m_conn_set[conn_id];
        conn->m_response_q.push(resp_pkg);
        m_resp_conn_q.push(conn_id);
        uint64_t resp_cnt = 1;
        ssize_t s = write(m_resp_ev_fd, &(resp_cnt), sizeof(uint64_t));
        if (s != sizeof(uint64_t)) {
            printf("write resp event fd fail\n");
        }
    }
    else {
        printf("connection %s gone, drop resp\n", conn_id.c_str());
        delete resp_pkg;
    }
    return;
}

void RpcServer::stop() {
    m_stop = true;
    for(auto rw: m_worker_vec) {
        rw->stop();
    }
    for (auto th: m_thread_vec) {
        th->join();
    }
}

};
