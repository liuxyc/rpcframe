/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
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
, m_fd(-1)
, m_conn(nullptr)
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

}

RpcEventLooper::~RpcEventLooper() {
    RPC_LOG(RPC_LOG_LEV::DEBUG, "~RpcEventLooper()");

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

void RpcEventLooper::removeConnection() {
    std::lock_guard<std::mutex> mlock(m_mutex);
    delete m_conn;
    m_conn = nullptr;
    m_fd = -1;
    for (auto cb = m_cb_map.begin(); cb != m_cb_map.end(); ) {
        if (cb->second != nullptr) {
            cb->second->callback_safe(RpcStatus::RPC_DISCONNECTED, RawData());
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
        ev.events = EPOLLIN;
        ev.data.fd = m_fd;
        epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_fd, &ev);  
    }
    else {
        RPC_LOG(RPC_LOG_LEV::ERROR, "invalid fd");
    }
}

RpcStatus RpcEventLooper::sendReq(
        const std::string &service_name, 
        const std::string &method_name, 
        const RawData &request_data, 
        std::shared_ptr<RpcClientCallBack> cb_obj, 
        std::string &req_id) {

    if (request_data.size() > m_client->getConfig().m_max_req_size) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "send data too large %lu", request_data.size());
        return RpcStatus::RPC_REQ_TOO_LARGE;
    }
    m_mutex.lock();
    if (m_conn == nullptr) {
        if (!connect() || m_conn == nullptr) {
            if (cb_obj != nullptr) {
                cb_obj->callback_safe(RpcStatus::RPC_SEND_FAIL, RawData());
            }
            m_mutex.unlock();
            return RpcStatus::RPC_SEND_FAIL;
        }
    }
    //gen readable request_id 
    std::stringstream ssm;
    ssm << (int)getpid()
        << "_" << std::this_thread::get_id() 
        << "_" << m_conn->getFd() 
        << "_" << std::time(nullptr)
        << "_" << m_host_ip
        << "_" << m_req_seqid;
    m_mutex.unlock();
    req_id = ssm.str();
    std::time_t tm_id;
    uint32_t cb_timeout = 0;
    if (cb_obj != nullptr) {
        //if has timeout set, put this callback to timeout map
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
    RpcStatus send_ret = RpcStatus::RPC_SEND_FAIL;
    m_mutex.lock();
    if (m_conn != nullptr) {
        ++m_req_seqid;
        send_ret = m_conn->sendReq(service_name, method_name, request_data, req_id, (cb_obj == nullptr), cb_timeout);
    }
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
            m_cb_map.erase(req_id);
            cb_obj->callback_safe(send_ret, RawData());
            m_mutex.unlock();
        }
    }
    return send_ret;

}

std::shared_ptr<RpcClientCallBack> RpcEventLooper::getCb(const std::string &req_id) {
    std::lock_guard<std::mutex> mlock(m_mutex);
    auto cb_iter = m_cb_map.find(req_id);
    if ( cb_iter != m_cb_map.end()) {
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
        if(m_cb_map.empty()) {
          break;
        }
      }
      sleep(1);
    }
}

void RpcEventLooper::removeCb(const std::string &req_id) {
    std::lock_guard<std::mutex> mlock(m_mutex);
    m_cb_map.erase(req_id);
}

void RpcEventLooper::dealTimeoutCb() {
    std::lock_guard<std::mutex> mlock(m_mutex);
    if (!m_cb_timer_map.empty()) {
        //search timeout cb
        for(auto cb_timer_it = m_cb_timer_map.begin(); 
                cb_timer_it != m_cb_timer_map.end();) {
            auto cur_it = cb_timer_it++;
            std::string reqid = cur_it->second;
            auto cb_iter = m_cb_map.find(reqid);
            if (cb_iter != m_cb_map.end()) { 
                std::shared_ptr<RpcClientCallBack> cb = cb_iter->second;
                if(cb != nullptr) {
                    std::time_t tm = cur_it->first;
                    if(std::time(nullptr) > tm) {
                        //found a timeout cb
                        //RPC_LOG(RPC_LOG_LEV::WARNING, "%s timeout", cb->getReqId().c_str());
                        cb->callback_safe(RpcStatus::RPC_CB_TIMEOUT, RawData());
                        m_cb_map.erase(cb_iter);
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
}

void RpcEventLooper::run() {
    prctl(PR_SET_NAME, "RpcClient", 0, 0, 0); 
    struct epoll_event events[_MAX_SOCKFD_COUNT];  
    while(1) {
        if (m_stop) {
            RPC_LOG(RPC_LOG_LEV::INFO, "RpcEventLooper stoped");
            removeConnection();
            break;
        }

        int nfds = epoll_wait(m_epoll_fd, events, _MAX_SOCKFD_COUNT, 1000);  
        if (m_stop) {
            RPC_LOG(RPC_LOG_LEV::INFO, "RpcEventLooper stoped");
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
                //pkgret: <int, pkgptr>
                //int: -1 get pkg data fail
                //      0 get pkg data success(may partial data)
                //int: 0 
                //  pkgptr: nullptr, partial data
                //  pkgptr: not nullptr, full pkg data 
                if( pkgret.first < 0 )  
                {  
                    RPC_LOG(RPC_LOG_LEV::WARNING, "rpc client socket disconnected: %d", client_socket);  
                    struct epoll_event event_del;  
                    event_del.data.fd = client_socket;
                    event_del.events = 0;  
                    epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, event_del.data.fd, &event_del);
                    removeConnection();
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
    close(m_epoll_fd);
}

bool RpcEventLooper::connect() {
    m_fd = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(m_fd < 0)
    {
        RPC_LOG(RPC_LOG_LEV::ERROR, "socket creation failed");
        return false;
    }

    if (noBlockConnect(m_fd, m_client->getConfig().m_hostname.c_str(), 
                       m_client->getConfig().m_port, m_client->getConfig().m_connect_timeout) == -1)
    {
        RPC_LOG(RPC_LOG_LEV::ERROR, "Connect error.");
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
        RPC_LOG(RPC_LOG_LEV::ERROR, "gethostbyname fail");
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
        RPC_LOG(RPC_LOG_LEV::ERROR, "unblock connect not support");
        ::close(sockfd);
        return -1;
    }
    fd_set writefds;
    struct timeval timeout;//connect time out
    FD_ZERO(&writefds);
    FD_SET(sockfd, &writefds);
    timeout.tv_sec = timeoutv;
    timeout.tv_usec = 0;
    ret = ::select(sockfd+1, nullptr, &writefds, nullptr, &timeout);
    if(ret <= 0) {
      if(ret == 0) { 
        RPC_LOG(RPC_LOG_LEV::ERROR, "connect %s time out", hostname);
        close(sockfd);
        return -1;
      }
      else {
        RPC_LOG(RPC_LOG_LEV::ERROR, "select connect host %s error:%s", hostname, strerror(errno));
      }
    }
    if(!FD_ISSET(sockfd, &writefds)) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "no events on sockfd found");
        close(sockfd);
        return -1;
    }
    int error = 0;
    socklen_t length = sizeof(error);
    if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &length) < 0) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "get socket option error");
        close(sockfd);
        return -1;
    }
    if(error != 0 ) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "connect host %s error:%s", hostname, strerror(error));
        close(sockfd);
        return -1;
    }
    //set socket back to block
    fcntl(sockfd, F_SETFL, fdopt); 
    return sockfd;
}
};
