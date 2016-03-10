/*
*   Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc gmail com>
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
    m_thread_vec.emplace_back(new std::thread(&RpcEventLooper::run, m_ev));

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

RpcStatus RpcClient::call(const std::string &method_name, const std::string &request_data, std::string &response_data, uint32_t timeout) {
    std::shared_ptr<RpcClientBlocker> rb(new RpcClientBlocker(timeout));
    std::string req_id;
    RpcStatus ret_st = m_ev->sendReq(m_servicename, method_name, request_data, rb, req_id);
    if (ret_st == RpcStatus::RPC_SEND_OK) {
        std::pair<RpcStatus, std::string> ret_p = rb->wait();
        response_data = ret_p.second;
        ret_st = ret_p.first;
    }
    else {
        ret_st = RpcStatus::RPC_SEND_FAIL;
    }
    m_ev->removeCb(req_id);
    
    return ret_st;
}

RpcStatus RpcClient::async_call(const std::string &method_name, const std::string &request_data, uint32_t timeout, std::shared_ptr<RpcClientCallBack> cb_obj) {
    std::string req_id;
    int test_heap = 0;
    if ((long)&test_heap < (long)(cb_obj.get())) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "[ERROR]please alloc cb_obj from heap!!!\n");
        return RpcStatus::RPC_SEND_FAIL;
    }
    if (cb_obj != nullptr) {
        cb_obj->setTimeout(timeout);
    }

    return m_ev->sendReq(m_servicename, method_name, request_data, cb_obj, req_id);
}

};
