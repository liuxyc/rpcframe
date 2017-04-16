/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include "RpcEventLooper.h"

#include <arpa/inet.h>  
#include <fcntl.h>  
#include <netdb.h>
#include <netinet/tcp.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/epoll.h>  
#include <sys/socket.h>  
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <sys/prctl.h>

#include <sstream>

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
, m_req_seqid(0)
, m_thread_num(thread_num)
{
    if(!getHostIp(m_host_ip)) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "[ERROR]get hostip fail!");
    }
    m_epoll_fd = epoll_create(_MAX_SOCKFD_COUNT);  
    //set noblock
    int opts = O_NONBLOCK;  
    if(fcntl(m_epoll_fd,F_SETFL,opts)<0)  
    {  
        RPC_LOG(RPC_LOG_LEV::ERROR, "set epoll fd noblock fail");
    }  

    for(int i = 0; i < m_thread_num; ++i) {
        RpcClientWorker *worker = new RpcClientWorker(this);
        std::thread *worker_th = new std::thread(&RpcClientWorker::run, worker);
        m_worker_vec.push_back(worker);
        m_thread_vec.push_back(worker_th);
    }
    for(auto &ep: client->getConfig().m_eps) {
        RpcClientConn *conn = new RpcClientConn(ep, m_client->getConfig().m_connect_timeout, this);
        m_ep_conn_map[ep] = conn;
        conn->connect();

    }

}

RpcEventLooper::~RpcEventLooper() {
    RPC_LOG(RPC_LOG_LEV::DEBUG, "~RpcEventLooper()");
    for(auto &ec:m_ep_conn_map) {
        delete ec.second;
    }

}

void RpcEventLooper::stop() {
    m_stop = true;
    for(int i = 0; i < m_thread_num; ++i) {
        m_worker_vec[i]->stop();
    }
    for(int i = 0; i < m_thread_num; ++i) {
        m_thread_vec[i]->join();
        delete m_worker_vec[i];
        delete m_thread_vec[i];
    }
}

void RpcEventLooper::removeConnection(int fd, RpcClientConn *conn) {
    std::lock_guard<std::mutex> mlock(m_mutex);
    struct epoll_event event_del;  
    event_del.data.fd = fd;
    event_del.events = 0;  
    epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, event_del.data.fd, &event_del);
    conn->setInvalid();
    auto callbacks = m_conn_cb_map.find(conn);
    if(callbacks != m_conn_cb_map.end()) {
        for (auto cb : callbacks->second) {
            if (cb != nullptr) {
                cb->callback_safe(RpcStatus::RPC_DISCONNECTED, RawData());
                m_id_cb_map.erase(cb->getReqId());
            }
        }
        callbacks->second.clear();
    }

}

void RpcEventLooper::removeAllConnections() {
    std::lock_guard<std::mutex> mlock(m_mutex);
    for (auto cb : m_id_cb_map) {
        if (cb.second != nullptr) {
            cb.second->callback_safe(RpcStatus::RPC_DISCONNECTED, RawData());
        }
    }
    m_id_cb_map.clear();
    m_cb_timer_map.clear();
}


void RpcEventLooper::addConnection(int fd, RpcClientConn *data)
{
    if (fd != -1) {
        struct epoll_event ev;  
        memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        ev.data.ptr = data;
        epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev);  
    }
    else {
        RPC_LOG(RPC_LOG_LEV::ERROR, "invalid fd");
    }
}

RpcClientConn *RpcEventLooper::tryGetAvaliableConn() 
{
    std::lock_guard<std::mutex> mlock(m_mutex);
    int max_ep_num = m_client->getConfig().m_eps.size();
    if(max_ep_num == 0) {
        return nullptr;
    }
    int n = m_req_seqid % max_ep_num;
    RpcClientConn *conn = m_ep_conn_map[m_client->getConfig().m_eps[n]];
    int try_times = 0;
    while(!conn->isValid()) {
        if (conn->shouldRetry() && conn->connect()) {
            return conn;
        }
        if(try_times >= max_ep_num) {
            //if all connection is invalid, just return the last selected
            break;
        }
        //select next
        conn = m_ep_conn_map[m_client->getConfig().m_eps[++n % max_ep_num]];
        ++try_times;
    }
    return conn;
}

RpcStatus RpcEventLooper::sendReq( const std::string &service_name, const std::string &method_name, 
        const RawData &request_data, std::shared_ptr<RpcClientCallBack> cb_obj, std::string &req_id) 
{
    //data limit check
    if (request_data.size() > m_client->getConfig().m_max_req_size) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "send data too large %lu", request_data.size());
        return RpcStatus::RPC_REQ_TOO_LARGE;
    }
    
    RpcClientConn *conn = tryGetAvaliableConn();
    if (conn == nullptr) {
        //no avaliable connection
        RPC_LOG(RPC_LOG_LEV::ERROR, "no avaliable connection");
        if (cb_obj != nullptr) {
            cb_obj->callback_safe(RpcStatus::RPC_SEND_FAIL, RawData());
        }
        return RpcStatus::RPC_SEND_FAIL;
    }
    //gen readable request_id 
    std::stringstream reqid_stream;
    reqid_stream << (int)getpid()
        << "_" << std::this_thread::get_id() 
        << "_" << conn->getFd() 
        << "_" << std::time(nullptr)
        << "_" << m_host_ip
        << "_" << m_req_seqid;
    req_id = reqid_stream.str();
    std::time_t tm_id;
    uint32_t cb_timeout = 0;
    if (cb_obj != nullptr) {
        //if has timeout set, put this callback to timeout map
        cb_obj->setReqId(req_id);
        m_mutex.lock();
        m_id_cb_map.insert(std::make_pair(req_id, cb_obj));
        m_mutex.unlock();
        cb_timeout = cb_obj->getTimeout();
        if( cb_timeout > 0) {
            tm_id = std::time(nullptr) + cb_timeout;
            m_mutex.lock();
            m_cb_timer_map.insert(std::make_pair(tm_id, req_id));
            m_mutex.unlock();
        }
    }
    RpcStatus send_ret = RpcStatus::RPC_SEND_FAIL;
    m_mutex.lock();
    ++m_req_seqid;
    send_ret = conn->sendReq(service_name, method_name, request_data, req_id, (cb_obj == nullptr), cb_timeout);
    m_mutex.unlock();
    if (send_ret == RpcStatus::RPC_SEND_OK) {
        RPC_LOG(RPC_LOG_LEV::DEBUG, "send %s", req_id.c_str());
    }
    else {
        RPC_LOG(RPC_LOG_LEV::ERROR, "send fail %s", req_id.c_str());
        if (cb_obj != nullptr) {
            m_mutex.lock();
            if( cb_timeout > 0) {
                m_cb_timer_map.erase(tm_id);
            }
            m_id_cb_map.erase(req_id);
            cb_obj->callback_safe(send_ret, RawData());
            m_mutex.unlock();
        }
    }
    return send_ret;

}

std::shared_ptr<RpcClientCallBack> RpcEventLooper::getCb(const std::string &req_id) {
    std::lock_guard<std::mutex> mlock(m_mutex);
    auto cb_iter = m_id_cb_map.find(req_id);
    if ( cb_iter != m_id_cb_map.end()) {
        return cb_iter->second;
    }
    else {
        return nullptr;
    }
}

void RpcEventLooper::waitAllCBDone(uint32_t timeout) {
    while(timeout) {
      {
        std::lock_guard<std::mutex> mlock(m_mutex);
        if(m_id_cb_map.empty()) {
          break;
        }
      }
      sleep(1);
      RPC_LOG(RPC_LOG_LEV::WARNING, "waiting callback done");
    }
}

void RpcEventLooper::removeCb(const std::string &req_id) {
    std::lock_guard<std::mutex> mlock(m_mutex);
    m_id_cb_map.erase(req_id);
}

void RpcEventLooper::dealTimeoutCb() {
    std::lock_guard<std::mutex> mlock(m_mutex);
    if (m_cb_timer_map.empty()) {
        return;
    }
    //search timeout cb
    for(auto cb_timer_it = m_cb_timer_map.begin(); 
            cb_timer_it != m_cb_timer_map.end();) {
        auto cur_it = cb_timer_it++;
        std::string reqid = cur_it->second;
        auto cb_iter = m_id_cb_map.find(reqid);
        if (cb_iter != m_id_cb_map.end()) { 
            std::shared_ptr<RpcClientCallBack> cb = cb_iter->second;
            if(cb != nullptr) {
                std::time_t tm = cur_it->first;
                if(std::time(nullptr) > tm) {
                    //found a timeout cb
                    //RPC_LOG(RPC_LOG_LEV::WARNING, "%s timeout", cb->getReqId().c_str());
                    cb->callback_safe(RpcStatus::RPC_CB_TIMEOUT, RawData());
                    m_id_cb_map.erase(cb_iter);
                }
                else {
                    //got item not timeout, stop search
                    break;
                }
            }
            else {
                RPC_LOG(RPC_LOG_LEV::ERROR, "found timeout nullptr cb");
            }
        }
        m_cb_timer_map.erase(cur_it);
    }
}

void RpcEventLooper::run() {
    prctl(PR_SET_NAME, "RpcClient", 0, 0, 0); 
    struct epoll_event events[_MAX_SOCKFD_COUNT];  
    while(1) {
        if (m_stop) {
            removeAllConnections();
            break;
        }

        int nfds = epoll_wait(m_epoll_fd, events, _MAX_SOCKFD_COUNT, 1000);  
        if (m_stop) {
            removeAllConnections();
            break;
        }

        //deal async call timeout
        //FIXME:dealTimeoutCb() may block event loop
        dealTimeoutCb();

        for (int i = 0; i < nfds; i++) 
        {  
            int client_socket = events[i].data.fd;  
            RpcClientConn *conn = (RpcClientConn *)(events[i].data.ptr);
            if (events[i].events & EPOLLIN)//data come in
            {  
                pkg_ret_t pkgret = conn->getResponse();
                //pkgret: <int, pkgptr>
                //int: -1 get pkg data fail
                //      0 get pkg data success(may partial data)
                //int: 0 
                //  pkgptr: nullptr, partial data
                //  pkgptr: not nullptr, full pkg data 
                if( pkgret.first < 0 )  
                {  
                    RPC_LOG(RPC_LOG_LEV::WARNING, "rpc client socket disconnected: %d", client_socket);  
                    removeConnection(client_socket, conn);
                }  
                else 
                {  
                    if (pkgret.second != nullptr) {
                        //got a full response, put to worker queue
                        m_response_q.push(pkgret.second);
                    }
                }  
            }  
            else  
            {  
                RPC_LOG(RPC_LOG_LEV::WARNING, "EPOLL ERROR");
                epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, client_socket, &events[i]);  
            }  
        }  
        //RPC_LOG(RPC_LOG_LEV::DEBUG, "epoll loop");
    }
    RPC_LOG(RPC_LOG_LEV::INFO, "RpcEventLooper stoped");
    close(m_epoll_fd);
}

};
