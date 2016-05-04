/*
*   Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc gmail com>
*   All rights reserved.
*  
*/
#include "RpcClient.h"

#include <thread>

#include "RpcClientBlocker.h"
#include "RpcEventLooper.h"
#include "util.h"
#include "RpcPackage.h"

namespace rpcframe
{

RpcClientConfig::RpcClientConfig(std::pair<const char *, int> &endpoint)
: m_thread_num(1)
, m_hostname(endpoint.first)
, m_port(endpoint.second)
, m_connect_timeout(3)
, m_max_req_size(1024 * 1024 * 128)
{
    
    
}

RpcClientConfig::~RpcClientConfig()
{
   
}

void RpcClientConfig::setThreadNum(uint32_t thread_num)
{
    if (thread_num > 0) {
        m_thread_num = thread_num;
    }
}

void RpcClientConfig::setMaxReqPkgSize(uint32_t max_req_size)
{
  m_max_req_size = max_req_size;

}

uint32_t RpcClientConfig::getThreadNum()
{
    return m_thread_num;
}


RpcClient::RpcClient(RpcClientConfig &cfg, const std::string &service_name)
: m_cfg(cfg)
, m_isConnected(false)
, m_fd(-1)
, m_servicename(service_name)
, m_ev(new RpcEventLooper(this, cfg.getThreadNum()))
{
    m_thread_vec.emplace_back(new std::thread(&RpcEventLooper::run, m_ev.get()));

}

RpcClient::~RpcClient() {
    RPC_LOG(RPC_LOG_LEV::DEBUG, "~RpcClient()");
    m_ev->stop();
    for(auto &th: m_thread_vec) {
        th->join();
    }

}

const RpcClientConfig &RpcClient::getConfig() {
    return m_cfg;
}

RpcStatus RpcClient::call(const std::string &method_name, const RawData &request_data, RawData &response_data, uint32_t timeout) {
    std::shared_ptr<RpcClientBlocker> rb(new RpcClientBlocker(timeout));
    std::string req_id;
    RpcStatus ret_st = m_ev->sendReq(m_servicename, method_name, request_data, rb, req_id);
    if (ret_st == RpcStatus::RPC_SEND_OK) {
        ret_st = rb->wait(req_id, response_data);
    }
    m_ev->removeCb(req_id);
    
    return ret_st;
}

RpcStatus RpcClient::async_call(const std::string &method_name, const RawData &request_data, uint32_t timeout, std::shared_ptr<RpcClientCallBack> cb_obj) {
    std::string req_id;
    if (cb_obj != nullptr) {
        cb_obj->setTimeout(timeout);
    }

    return m_ev->sendReq(m_servicename, method_name, request_data, cb_obj, req_id);
}

void RpcClient::waitAllCBDone(uint32_t timeout) 
{
  m_ev->waitAllCBDone(timeout);
}
};
