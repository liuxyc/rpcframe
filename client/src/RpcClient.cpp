/*
*   Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc gmail com>
*   All rights reserved.
*  
*/
#include <thread>

#include "RpcClient.h"
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
{
    m_ev = new RpcEventLooper(this, cfg.getThreadNum());
    std::thread *th = new std::thread(&RpcEventLooper::run, m_ev);
    m_thread_vec.push_back(th);

}

RpcClient::~RpcClient() {
    printf("~RpcClient()\n");
    m_ev->stop();
    for(auto th: m_thread_vec) {
        th->join();
        delete th;
    }
    delete m_ev;

}

const RpcClientConfig &RpcClient::getConfig() {
    return m_cfg;
}

RpcStatus RpcClient::call(const std::string &method_name, const std::string &request_data, std::string &response_data, uint32_t timeout) {
    RpcClientBlocker *rb = new RpcClientBlocker(timeout);
    std::string req_id;
    RpcStatus ret_st = m_ev->sendReq(m_servicename, method_name, request_data, rb, req_id);
    if (ret_st == RpcStatus::RPC_SEND_OK) {
        std::pair<RpcStatus, std::string> ret_p = rb->wait();
        response_data = ret_p.second;
        ret_st = ret_p.first;
        if(ret_st == RpcStatus::RPC_CB_TIMEOUT) {
            //delete rb will cause race condition if the real response back, 
            //so, call m_ev->timeoutCb will send a fake response
            //let Worker remove the callback instance
            rb->markTimeout();
            rb->setType("timeoutB");
            m_ev->timeoutCb(req_id);
        }
        else {
            m_ev->removeCb(req_id);
            delete rb;
        }
    }
    else {
        ret_st = RpcStatus::RPC_SEND_FAIL;
    }
    
    return ret_st;
}

RpcStatus RpcClient::async_call(const std::string &method_name, const std::string &request_data, uint32_t timeout, RpcClientCallBack *cb_obj) {
    std::string req_id;
    int test_heap = 0;
    if ((long)&test_heap < (long)cb_obj) {
        printf("[ERROR]please alloc cb_obj from heap!!!\n");
        return RpcStatus::RPC_SEND_FAIL;
    }
    if (cb_obj != NULL) {
        cb_obj->setTimeout(timeout);
    }

    return m_ev->sendReq(m_servicename, method_name, request_data, cb_obj, req_id);
}

};
