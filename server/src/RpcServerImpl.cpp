/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include "RpcServerImpl.h"

#include <sys/socket.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>
#include <sys/prctl.h>

#include <thread>
#include <chrono>
#include <vector>
#include <utility>

#include "util.h"
#include "RpcWorker.h"
#include "RpcHttpServer.h"
#include "RpcStatusService.h"
#include "IService.h"
#include "RpcServerConfig.h"
#include "RpcServerConnWorker.h"

namespace rpcframe
{

RpcServerImpl::RpcServerImpl(RpcServerConfig &cfg)
: m_cfg(cfg)
, m_request_q(cfg.m_max_req_qsize)
, m_listen_socket(-1)
, m_stop(false)
, m_conn_num(0)
, m_http_server(nullptr)
, avg_req_wait_time(0)
, avg_resp_wait_time(0)
, avg_call_time(0)
, max_call_time(0)
, total_req_num(0)
, total_resp_num(0)
, total_call_num(0)
, rejected_conn(0)
, req_inqueue_fail(0)
, resp_inqueue_fail(0)
{
    for(uint32_t i = 0; i < m_cfg.getConnThreadNum(); ++i) {
      std::string connworkername("connworker_");
      m_connworker.push_back(new RpcServerConnWorker(this, connworkername.append(std::to_string(i)).c_str(), &m_request_q));
    }

    addWorkers(m_cfg.getThreadNum());
    if (cfg.getHttpPort() != -1) {
        m_http_server = new RpcHttpServer(cfg, this);
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
    for(auto cw: m_connworker) {
        delete cw;
    }
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
    m_listen_socket = socket(AF_INET,SOCK_STREAM,0);  
    if ( 0 > m_listen_socket )  
    {  
        RPC_LOG(RPC_LOG_LEV::ERROR, "socket error!");  
        return false;  
    }  
    
    sockaddr_in listen_addr;  
    listen_addr.sin_family = AF_INET;  
    listen_addr.sin_port = htons(m_cfg.m_port);  
    std::string hostip;
    if(!getHostIpByName(hostip, m_cfg.m_hostname.c_str())) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "gethostbyname fail");
    }
    listen_addr.sin_addr.s_addr = inet_addr(hostip.c_str());  
    
    int ireuseadd_on = 1;
    setsockopt(m_listen_socket, SOL_SOCKET, SO_REUSEADDR, &ireuseadd_on, sizeof(ireuseadd_on) );  
    //set nonblock
    int opts = O_NONBLOCK;  
    if(fcntl(m_listen_socket, F_SETFL, opts) < 0)  
    {  
      RPC_LOG(RPC_LOG_LEV::ERROR, "set listen fd nonblock fail");  
      return false;
    }  

    if (bind(m_listen_socket, (sockaddr *) &listen_addr, sizeof (listen_addr) ) != 0 )  
    {  
        RPC_LOG(RPC_LOG_LEV::ERROR, "bind %s:%d error", hostip.c_str(), m_cfg.m_port);  
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
    return true;
}

bool RpcServerImpl::start() {
    if (m_cfg.getHttpPort() != -1) {
        m_http_server->start();
    }
    if(!startListen()) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "start listen failed");
        return false;
    }
    std::vector<std::unique_ptr<std::thread> > thread_vec;
    for(auto connw: m_connworker) {
      thread_vec.push_back(std::unique_ptr<std::thread>(new std::thread([connw, this](){
            connw->start(this->m_listen_socket);
          })));
    }
    for(auto &th: thread_vec) {
      th->join();
    }
    return true;
}

void RpcServerImpl::stop() {
    if (m_stop) {
      return;
    }
    m_stop = true;
    for(auto connw: m_connworker) {
      connw->stop();
    }
    m_http_server->stop();
    delete m_http_server;
    removeWorkers(m_worker_vec.size());
    RPC_LOG(RPC_LOG_LEV::INFO, "stoped");
}

void RpcServerImpl::calcReqQTime(uint64_t req_time)
{
  {
    std::lock_guard<SpinLock> mlock(m_stat_lock);
    if (avg_req_wait_time == 0) {
      avg_req_wait_time = req_time;
    }
    else {
      avg_req_wait_time = ((avg_req_wait_time * total_req_num) + req_time) / (total_req_num + 1);
    }
    ++total_req_num;
  }
  RPC_LOG(RPC_LOG_LEV::DEBUG, "avg req wait: %llu ms", avg_req_wait_time);
}


void RpcServerImpl::calcRespQTime(uint64_t resp_time)
{
  {
    std::lock_guard<SpinLock> mlock(m_stat_lock);
    if (avg_resp_wait_time == 0) {
      avg_resp_wait_time = resp_time;
    }
    else {
      avg_resp_wait_time = ((avg_resp_wait_time * total_resp_num) + resp_time) / (total_resp_num + 1);
    }
    ++total_resp_num;
  }
  RPC_LOG(RPC_LOG_LEV::DEBUG, "avg resp wait: %llu ms", avg_resp_wait_time);

}

void RpcServerImpl::calcCallTime(uint64_t call_time)
{
  {
    std::lock_guard<SpinLock> mlock(m_stat_lock);
    if (avg_call_time == 0) {
      avg_call_time = call_time;
    }
    else {
      avg_call_time = ((avg_call_time * total_call_num) + call_time) / (total_call_num + 1);
    }
    ++total_call_num;
    if(call_time > max_call_time) {
      max_call_time = call_time;
    }
  }
  RPC_LOG(RPC_LOG_LEV::DEBUG, "avg call time: %llu ms", avg_call_time);
}

void RpcServerImpl::IncRejectedConn()
{
  std::lock_guard<SpinLock> mlock(m_stat_lock);
  ++rejected_conn;
}

void RpcServerImpl::IncReqInQFail()
{
  std::lock_guard<SpinLock> mlock(m_stat_lock);
  ++req_inqueue_fail;
}

void RpcServerImpl::IncRespInQFail()
{
  std::lock_guard<SpinLock> mlock(m_stat_lock);
  ++resp_inqueue_fail;

}

const RpcServerConfig *RpcServerImpl::getConfig()
{
  return &m_cfg;
}

std::vector<RpcServerConnWorker *> &RpcServerImpl::getConnWorker() 
{
  return m_connworker;
}

void RpcServerImpl::DecConnCount()
{
  m_conn_num--;
}

void RpcServerImpl::IncConnCount()
{
  m_conn_num++;
}

uint64_t RpcServerImpl::GetConnCount()
{
  return m_conn_num.load();
}
};
