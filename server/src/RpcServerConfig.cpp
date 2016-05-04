/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include "RpcServerConfig.h"

#include <string>
#include <thread>
//#include "RpcServerImpl.h"

namespace rpcframe
{
RpcServerConfig::RpcServerConfig(std::pair<const char *, int> &endpoint)
: m_thread_num(std::thread::hardware_concurrency())
, m_hostname(endpoint.first)
, m_port(endpoint.second)
, m_max_conn_num(1024 * 10)
, m_max_req_size(1024 * 1024 * 128)
, m_http_port(8000)
, m_http_thread_num(std::thread::hardware_concurrency())
, m_max_req_qsize(10 * 1024 * 1024)
, m_conn_thread_num(4)
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

void RpcServerConfig::setConnThreadNum(uint16_t thread_num)
{
  if (thread_num > 0) {
    m_conn_thread_num = thread_num;
  }
}

uint16_t RpcServerConfig::getConnThreadNum()
{
  return m_conn_thread_num;
}

void RpcServerConfig::setMaxConnection(uint32_t max_conn_num)
{
    m_max_conn_num = max_conn_num;
}

void RpcServerConfig::setMaxReqPkgSize(uint32_t max_req_size)
{
    m_max_req_size = max_req_size;
}

void RpcServerConfig::setMaxReqQSize(uint32_t max_req_qsize)
{
    m_max_req_qsize = max_req_qsize;
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


};
