/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include "RpcServerImpl.h"

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
#include <sys/prctl.h>

#include <thread>
#include <chrono>

#include "RpcServerConn.h"
#include "util.h"
#include "RpcPackage.h"
#include "RpcWorker.h"
#include "RpcHttpServer.h"
#include "RpcStatusService.h"

#define RPC_MAX_SOCKFD_COUNT 65535 

namespace rpcframe
{

RpcServerConfig::RpcServerConfig(std::pair<const char *, int> &endpoint)
: m_thread_num(std::thread::hardware_concurrency())
, m_hostname(endpoint.first)
, m_port(endpoint.second)
, m_max_conn_num(1024 * 10)
, m_http_port(8000)
, m_http_thread_num(std::thread::hardware_concurrency())
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

void RpcServerConfig::enableHttp(int port, int thread_num)
{
    m_http_port = port;
    m_http_thread_num = thread_num;
}

void RpcServerConfig::disableHttp()
{
    m_http_port = -1;
}

int RpcServerConfig::getHttpPort()
{
    return m_http_port;
}

RpcServerImpl::RpcServerImpl(RpcServerConfig &cfg)
: m_cfg(cfg)
, m_seqid(0)
, m_epoll_fd(-1)
, m_listen_socket(-1)
, m_resp_ev_fd(-1)
, m_stop(false)
, m_http_server(nullptr)
, avg_req_wait_time(0)
, avg_resp_wait_time(0)
, avg_call_time(0)
, max_call_time(0)
, total_req_num(0)
, total_resp_num(0)
, total_call_num(0)
{
    addWorkers(m_cfg.getThreadNum());
    if (cfg.getHttpPort() != -1) {
        m_http_server = new RpcHttpServer(cfg, this);
        m_http_server->start();
    }
    RpcStatusService *ss = new RpcStatusService(this); 
    m_service_map["status"] = ss;
}

RpcServerImpl::~RpcServerImpl() {
    stop();
    for(auto rw: m_worker_vec) {
        delete rw;
    }
    delete m_service_map["status"];
}

bool RpcServerImpl::addWorkers(const uint32_t numbers) 
{
    for(uint32_t i = 0; i < numbers; ++i) {
        try {
            RpcWorker *rw = new RpcWorker(&m_request_q, this);
            m_worker_vec.push_back(rw);
        } catch (...) {
            return false;
        }
    }
    return true;
}

bool RpcServerImpl::removeWorkers(const uint32_t numbers)
{
    if (numbers > m_worker_vec.size() || m_worker_vec.size() == 0) {
        return false;
    }
    for(uint32_t i = 0; i < numbers; ++i) {
        auto last_worker = m_worker_vec.rbegin();
        (*last_worker)->stop();
        delete *last_worker;
        m_worker_vec.pop_back();
    }
    return true;
}

bool RpcServerImpl::addService(const std::string &name, IService *p_service)
{
    if (m_service_map.find(name) != m_service_map.end()) {
        return false;
    }
    m_service_map[name] = p_service;
    m_methodstatus_map.emplace(name, RpcMethodStatus());
    return true;
}

IService *RpcServerImpl::getService(const std::string &name)
{
    auto service_iter = m_service_map.find(name);
    if (service_iter != m_service_map.end()) {
        return service_iter->second;
    }
    else {
        return nullptr;
    }
}


bool RpcServerImpl::startListen() {

    m_epoll_fd = epoll_create(RPC_MAX_SOCKFD_COUNT);  
    if( m_epoll_fd == -1) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "epoll_create fail %s", strerror(errno));
        return false;
    }
    //set nonblock
    int opts = O_NONBLOCK;  
    if(fcntl(m_epoll_fd, F_SETFL, opts) < 0)  
    {  
        RPC_LOG(RPC_LOG_LEV::ERROR, "set epool fd nonblock fail");  
        return false;
    }  

    m_listen_socket = socket(AF_INET,SOCK_STREAM,0);  
    if ( 0 > m_listen_socket )  
    {  
        RPC_LOG(RPC_LOG_LEV::ERROR, "socket error!");  
        return false;  
    }  
    
    sockaddr_in listen_addr;  
    listen_addr.sin_family = AF_INET;  
    listen_addr.sin_port = htons(m_cfg.m_port);  
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
    std::string hostip;
    if(!getHostIpByName(hostip, m_cfg.m_hostname.c_str())) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "gethostbyname fail");
    }
    listen_addr.sin_addr.s_addr = inet_addr(hostip.c_str());  
    
    int ireuseadd_on = 1;
    setsockopt(m_listen_socket, SOL_SOCKET, SO_REUSEADDR, &ireuseadd_on, sizeof(ireuseadd_on) );  
    
    if (bind(m_listen_socket, (sockaddr *) &listen_addr, sizeof (listen_addr) ) != 0 )  
    {  
        RPC_LOG(RPC_LOG_LEV::ERROR, "bind error");  
        return false;  
    }  
    
    if (listen(m_listen_socket, 20) < 0 )  
    {  
        RPC_LOG(RPC_LOG_LEV::ERROR, "listen error!");  
        return false;  
    }  
    else {  
        RPC_LOG(RPC_LOG_LEV::INFO, "Listening on %s:%d", hostip.c_str(), m_cfg.m_port);  
    }  
    //listen socket epoll event
    struct epoll_event ev;  
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;  
    ev.data.fd = m_listen_socket;
    epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_listen_socket, &ev);  

    //listen resp_ev_fd, this event fd used for response data avaliable notification
    m_resp_ev_fd = eventfd(0, EFD_SEMAPHORE);  
    if (m_resp_ev_fd == -1) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "create event fd fail");  
        return false;
    }

    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;  
    ev.data.fd = m_resp_ev_fd;
    epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_resp_ev_fd, &ev);  

    return true;
}

void RpcServerImpl::onDataOut(const int fd) {
    auto conn_iter = m_conn_map.find(fd);
    if (conn_iter != m_conn_map.end()) {
        RpcServerConn *conn = conn_iter->second;
        PkgIOStatus sent_ret = conn->sendResponse();
        if (sent_ret == PkgIOStatus::FAIL ) {
            removeConnection(fd);
        }
        else if ( sent_ret == PkgIOStatus::PARTIAL ){
            //RPC_LOG(RPC_LOG_LEV::DEBUG, "OUT sent partial to %d", fd);
        }
        else {
            //RPC_LOG(RPC_LOG_LEV::DEBUG, "OUT sent to %d", fd);
            //send full resp, remove EPOLLOUT flag
            struct epoll_event event_mod;  
            memset(&event_mod, 0, sizeof(event_mod));
            event_mod.data.fd = fd;
            event_mod.events = EPOLLIN;
            epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, event_mod.data.fd, &event_mod);
        }
    }
}

bool RpcServerImpl::onDataOutEvent() {
    eventfd_t resp_cnt = -1;
    if (eventfd_read(m_resp_ev_fd, &resp_cnt) == -1) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "read resp event fail");
        return false;
    }

    std::string connid;
    //if the connection still sending data,
    //we put the event back, wait for next chance
    if (m_resp_conn_q.peak(connid)) {
        if (m_conn_set.find(connid) != m_conn_set.end()) {
            RpcServerConn *conn = m_conn_set[connid];
            if(conn->isSending()) {
                //RPC_LOG(RPC_LOG_LEV::DEBUG, "still sending pkg");
                //keep eventfd filled with the count of ready connection
                resp_cnt = 1;
                if( eventfd_write(m_resp_ev_fd, resp_cnt) == -1) {
                  RPC_LOG(RPC_LOG_LEV::ERROR, "write resp event fd fail");
                }
                return false;
            }
        }
    }

    if (m_resp_conn_q.pop(connid, 0)) {
        if (m_conn_set.find(connid) != m_conn_set.end()) {
            //we have resp data to send
            RpcServerConn *conn = m_conn_set[connid];
            //RPC_LOG(RPC_LOG_LEV::DEBUG, "conn %s resp queue len %lu", 
                    //conn->m_seqid.c_str(), 
                    //conn->m_response_q.size());
            PkgIOStatus sent_ret = conn->sendResponse();
            if (sent_ret == PkgIOStatus::FAIL ) {
                removeConnection(conn->getFd());
            }
            else if ( sent_ret == PkgIOStatus::PARTIAL ){
                //RPC_LOG(RPC_LOG_LEV::DEBUG, "sent partial to %d", conn->getFd());
                //send not finish, set EPOLLOUT flag on this fd, 
                //until this resp send finish
                struct epoll_event ev;  
                memset(&ev, 0, sizeof(ev));
                ev.events = EPOLLIN | EPOLLOUT;
                ev.data.fd = conn->getFd();
                epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, ev.data.fd, &ev);  
            }
            else {
                //RPC_LOG(RPC_LOG_LEV::DEBUG, "sent to %d\n", conn->getFd());
                //send full resp finish, try remove EPOLLOUT flag if it already set
                struct epoll_event event_mod;  
                memset(&event_mod, 0, sizeof(event_mod));
                event_mod.data.fd = conn->getFd();
                event_mod.events = EPOLLIN;
                epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, event_mod.data.fd, &event_mod);
            }
        }
    }
    return true;

}

void RpcServerImpl::onAccept() {
    //listen socket event
    sockaddr_in remote_addr;  
    int len = sizeof(remote_addr);  
    int new_client_socket = accept(m_listen_socket, 
            (sockaddr *)&remote_addr, 
            (socklen_t*)&len );  
    if ( new_client_socket < 0 ) {  
        RPC_LOG(RPC_LOG_LEV::ERROR, "accept fail %s, new_client_socket: %d", 
                strerror(errno), new_client_socket);  
    }  
    else {
        if( m_conn_set.size() >= m_cfg.m_max_conn_num) {
            RPC_LOG(RPC_LOG_LEV::ERROR, "conn number reach limit %d, close %d", m_cfg.m_max_conn_num, 
                    new_client_socket);  
            ::close(new_client_socket);
        }
        setSocketKeepAlive(new_client_socket);
        //NOTICE:do not use O_NONBLOCK, because we assume the first recv of pkglen 
        // must have 4 bytes at least
        //fcntl(new_client_socket, F_SETFL, fcntl(new_client_socket, F_GETFL) | O_NONBLOCK);
        struct epoll_event ev;  
        memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLIN ;
        ev.data.fd = new_client_socket;
        epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, new_client_socket, &ev);  
        m_seqid++;
        addConnection(new_client_socket, new RpcServerConn(new_client_socket, m_seqid));
        RPC_LOG(RPC_LOG_LEV::INFO, "new_client_socket: %d", new_client_socket);  
    }
}

void RpcServerImpl::onDataIn(const int fd) {
    //data come in
    RpcServerConn *conn = getConnection(fd);
    if (conn == nullptr) {
        //RPC_LOG(RPC_LOG_LEV::WARNING, "rpc server socket already disconnected: %d", fd);  
    }
    else {
        pkg_ret_t pkgret = conn->getRequest();
        if( pkgret.first < 0 )  
        {  
            RPC_LOG(RPC_LOG_LEV::WARNING, "rpc server socket disconnected: %d", fd);  
            removeConnection(fd);
        }  
        else 
        {  
            if (pkgret.second != nullptr) {
                pkgret.second->gen_time = std::chrono::system_clock::now();
                //got a full request, put to worker queue
                if ( !m_request_q.push(pkgret.second)) {
                    //queue fail, drop pkg
                    RPC_LOG(RPC_LOG_LEV::WARNING, "server queue fail, drop pkg");
                }
            }
        }  
    }
}

bool RpcServerImpl::start() {
    if(!startListen()) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "start listen failed");
    }

    struct epoll_event events[RPC_MAX_SOCKFD_COUNT];  
    while(!m_stop) {
        int nfds = epoll_wait(m_epoll_fd, events, RPC_MAX_SOCKFD_COUNT, 2000);  
        if ( nfds == -1) {
            if( m_stop ) {
              break;
            }
            RPC_LOG(RPC_LOG_LEV::ERROR, "epoll_pwait");
        }
        for (int i = 0; i < nfds; i++)  
        {  
            int client_socket = events[i].data.fd;  

            if (events[i].events & EPOLLOUT)
            {  
                onDataOut(client_socket);
            }

            //event fd, we have data to send
            if (events[i].events & EPOLLIN)
            {  
                if (client_socket == m_resp_ev_fd) {
                    if (!onDataOutEvent()) {
                        continue;
                    }
                }
                else if (client_socket == m_listen_socket) {
                    onAccept();
                }
                else {
                    onDataIn(client_socket);
                }
            }  

            if (events[i].events & EPOLLERR) {
                RPC_LOG(RPC_LOG_LEV::ERROR, "EPOLL ERROR");
            }
            if (events[i].events & EPOLLHUP) {
                RPC_LOG(RPC_LOG_LEV::ERROR, "EPOLL HUP");
            }
        }  
        //RPC_LOG(RPC_LOG_LEV::DEBUG, "server eloop request queue len %lu", m_request_q.size());
    }
    return true;
}

void RpcServerImpl::setSocketKeepAlive(int fd)
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

void RpcServerImpl::removeConnection(int fd)
{
    std::lock_guard<std::mutex> mlock(m_mutex);
    epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    auto conn_iter = m_conn_map.find(fd);
    if (conn_iter != m_conn_map.end()) {
        m_conn_set.erase(conn_iter->second->m_seqid);
        delete conn_iter->second;
        m_conn_map.erase(conn_iter);
    }
}

void RpcServerImpl::addConnection(int fd, RpcServerConn *conn)
{
    std::lock_guard<std::mutex> mlock(m_mutex);
    m_conn_set.emplace(conn->m_seqid, conn);
    m_conn_map.emplace(fd, conn);
}

RpcServerConn *RpcServerImpl::getConnection(int fd)
{
    std::lock_guard<std::mutex> mlock(m_mutex);
    auto conn_iter = m_conn_map.find(fd);
    if( conn_iter != m_conn_map.end()) {
        return conn_iter->second;
    }
    return nullptr;
}

void RpcServerImpl::pushResp(std::string conn_id, RespPkgPtr &resp_pkg)
{
    std::lock_guard<std::mutex> mlock(m_mutex);
    auto conn_iter = m_conn_set.find(conn_id);
    if (conn_iter != m_conn_set.end()) {
        RpcServerConn *conn = conn_iter->second;
        resp_pkg->gen_time = std::chrono::system_clock::now();
        if (!conn->m_response_q.push(resp_pkg)) {
            RPC_LOG(RPC_LOG_LEV::WARNING, "server resp queue fail, drop resp pkg");
            return;
        }
        m_resp_conn_q.push(conn_id);

        eventfd_t resp_cnt = 1;
        if( eventfd_write(m_resp_ev_fd, resp_cnt) == -1) {
            RPC_LOG(RPC_LOG_LEV::ERROR, "write resp event fd fail");
        }
    }
    else {
        RPC_LOG(RPC_LOG_LEV::WARNING, "connection %s gone, drop resp", conn_id.c_str());
    }
    return;
}

void RpcServerImpl::stop() {
    if (m_stop) {
      return;
    }
    m_stop = true;
    m_http_server->stop();
    delete m_http_server;
    removeWorkers(m_worker_vec.size());
    RPC_LOG(RPC_LOG_LEV::INFO, "RpcServer stoped");
}

void RpcServerImpl::calcReqQTime(uint64_t req_time)
{
    //TODO:spin lock
    std::lock_guard<std::mutex> mlock(m_stat_mutex);
    if (avg_req_wait_time == 0) {
        avg_req_wait_time = req_time;
    }
    else {
        avg_req_wait_time = ((avg_req_wait_time * total_req_num) + req_time) / (total_req_num + 1);
        ++total_req_num;
    }
    RPC_LOG(RPC_LOG_LEV::DEBUG, "avg req wait: %d ms", avg_req_wait_time);
}


void RpcServerImpl::calcRespQTime(uint64_t resp_time)
{
    //TODO:spin lock
    std::lock_guard<std::mutex> mlock(m_stat_mutex);
    if (avg_resp_wait_time == 0) {
        avg_resp_wait_time = resp_time;
    }
    else {
        avg_resp_wait_time = ((avg_resp_wait_time * total_resp_num) + resp_time) / (total_resp_num + 1);
        ++total_resp_num;
    }
    RPC_LOG(RPC_LOG_LEV::DEBUG, "avg resp wait: %d ms", avg_resp_wait_time);

}

void RpcServerImpl::calcCallTime(uint64_t call_time)
{
    //TODO:spin lock
    std::lock_guard<std::mutex> mlock(m_stat_mutex);
    if (avg_call_time == 0) {
        avg_call_time = call_time;
    }
    else {
        avg_call_time = ((avg_call_time * total_call_num) + call_time) / (total_call_num + 1);
        ++total_call_num;
    }
    if(call_time > max_call_time) {
        max_call_time = call_time;
    }
    RPC_LOG(RPC_LOG_LEV::DEBUG, "avg call time: %d ms", avg_call_time);

}


};
