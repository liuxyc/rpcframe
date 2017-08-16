/*
 * Copyright (c) 2015-2017, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include "RpcServerImpl.h"

#include <sys/socket.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>
#include <sys/prctl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <thread>
#include <chrono>
#include <vector>
#include <utility>

#include "util.h"
#include "RpcStatusService.h"
#include "IService.h"
#include "RpcServerConnWorker.h"

namespace rpcframe
{

RpcServerImpl::RpcServerImpl(RpcServerConfig &cfg)
//: m_http_server(nullptr)
: m_rpclisten(-1)
, m_httplisten(-1)
, m_cfg(cfg)
, m_stop(false)
, m_conn_num(0)
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
    m_statusSrv = new RpcStatusService(this); 

    m_worker_thread_pool = new ThreadPool<ReqPkgPtr, RpcWorker>(m_cfg.getThreadNum(), this);
    m_worker_thread_pool->setTaskQSize(cfg.m_max_req_qsize);
    std::vector<RpcWorker *> real_workers;
    m_worker_thread_pool->getWorkers(real_workers);
    for(auto w: real_workers) {
        w->addService("status", m_statusSrv, false);
    }

    for(uint32_t i = 0; i < m_cfg.getConnThreadNum(); ++i) {
      std::string connworkername("rpcconnworker_");
      m_rpcconnworker.push_back(new RpcServerConnWorker(this, connworkername.append(std::to_string(i)).c_str(), ConnType::RPC_CONN));
    }

    if (cfg.getHttpPort() != -1) {
        for(uint32_t i = 0; i < m_cfg.getConnThreadNum(); ++i) {
            std::string connworkername("httpconnworker_");
            m_httpconnworker.push_back(new RpcServerConnWorker(this, connworkername.append(std::to_string(i)).c_str(), ConnType::HTTP_CONN));
        }
        //m_http_server = new RpcHttpServer(cfg, this);
        //m_http_server->addService("status", m_statusSrv, false);
    }
}

RpcServerImpl::~RpcServerImpl() {
    stop();
    for(auto cw: m_rpcconnworker) {
      delete cw;
    }
    for(auto cw: m_httpconnworker) {
      delete cw;
    }
    delete m_statusSrv;
}

bool RpcServerImpl::pushReqToWorkers(ReqPkgPtr req) 
{
    return m_worker_thread_pool->addTask(req);
}


//IService *RpcServerImpl::getService(const std::string &name, void *worker)
//{
    //auto service_iter = m_service_map.find(std::make_pair(name, worker));
    //if (service_iter != m_service_map.end()) {
        //return service_iter->second;
    //}
    //else {
        //return nullptr;
    //}
//}


int RpcServerImpl::startListen(int port) {
    int listen_socket = socket(AF_INET,SOCK_STREAM,0);  
    if ( 0 > listen_socket )  
    {  
        RPC_LOG(RPC_LOG_LEV::ERROR, "socket error!");  
        return -1;  
    }  
    
    sockaddr_in listen_addr;  
    listen_addr.sin_family = AF_INET;  
    listen_addr.sin_port = htons(port);  
    std::string hostip;
    if(!getHostIpByName(hostip, m_cfg.m_hostname.c_str())) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "gethostbyname fail");
    }
    listen_addr.sin_addr.s_addr = inet_addr(hostip.c_str());  
    
    int ireuseadd_on = 1;
    //setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &ireuseadd_on, sizeof(ireuseadd_on) );   // linux kernel >=3.9
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &ireuseadd_on, sizeof(ireuseadd_on) );  
    //set nonblock
    int opts = O_NONBLOCK;  
    if(fcntl(listen_socket, F_SETFL, opts) < 0)  
    {  
      RPC_LOG(RPC_LOG_LEV::ERROR, "set listen fd nonblock fail");  
      return -1;
    }  

    if (bind(listen_socket, (sockaddr *) &listen_addr, sizeof (listen_addr) ) != 0 )  
    {  
        RPC_LOG(RPC_LOG_LEV::ERROR, "bind %s:%d error", hostip.c_str(), port);  
        return -1;  
    }  
    
    if (listen(listen_socket, 20) < 0 )  
    {  
        RPC_LOG(RPC_LOG_LEV::ERROR, "listen error!");  
        return -1;  
    }  
    else {  
        RPC_LOG(RPC_LOG_LEV::INFO, "Listening on %s:%d", hostip.c_str(), port);  
    }  
    return listen_socket;
}

bool RpcServerImpl::start() {
    if (m_cfg.getHttpPort() != -1) {
        if((m_httplisten = startListen(m_cfg.getHttpPort())) == -1) {
            RPC_LOG(RPC_LOG_LEV::ERROR, "start http listen failed");
            return false;
        }
        //m_http_server->start();
    }
    if((m_rpclisten = startListen(m_cfg.m_port)) == -1) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "start rpc listen failed");
        return false;
    }
    std::vector<std::unique_ptr<std::thread> > thread_vec;
    for(auto connw: m_rpcconnworker) {
      thread_vec.push_back(std::unique_ptr<std::thread>(new std::thread([connw, this](){
            connw->start(m_rpclisten);
          })));
    }
    for(auto connw: m_httpconnworker) {
      thread_vec.push_back(std::unique_ptr<std::thread>(new std::thread([connw, this](){
            connw->start(m_httplisten);
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
    for(auto connw: m_rpcconnworker) {
      connw->stop();
    }
    for(auto connw: m_httpconnworker) {
      connw->stop();
    }
    //m_http_server->stop();
    //delete m_http_server;
    delete m_worker_thread_pool;
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
  //RPC_LOG(RPC_LOG_LEV::DEBUG, "avg req wait: %llu ms", avg_req_wait_time);
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
  //RPC_LOG(RPC_LOG_LEV::DEBUG, "avg resp wait: %llu ms", avg_resp_wait_time);
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
  //RPC_LOG(RPC_LOG_LEV::DEBUG, "avg call time: %llu ms", avg_call_time);
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

std::vector<RpcServerConnWorker *> &RpcServerImpl::getRpcConnWorker() 
{
  return m_rpcconnworker;
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
