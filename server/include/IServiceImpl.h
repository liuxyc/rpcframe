/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include <unordered_map>
#include <string>
#include <vector>

#include "RpcDefs.h"
#include "IRpcRespBroker.h"
#include "RpcMethod.h"

namespace rpcframe {

class IServiceImpl
{
  public:
    RpcStatus runMethod(const std::string &method_name, 
        const RawData &req_data,
        IRpcRespBrokerPtr resp_broker, RpcMethodStatusPtr &method_status)
    {
        method_status = nullptr;
        auto method = m_method_map.find(method_name);
        if (method != m_method_map.end()) { 
            RPC_FUNC_T p_fun = method->second.m_func; 
            method_status = method->second.m_status;
            return p_fun(req_data, resp_broker); 
        }  
        else { 
            return RpcStatus::RPC_METHOD_NOTFOUND;  
        }  
    }

    void add_method(const std::string &method_name, const RPC_FUNC_T &func, bool allow_http) {
        m_method_map.emplace(method_name, rpcframe::RpcMethod(func, allow_http));
    }

    bool is_method_allow_http(const std::string &method_name) {
        auto m = m_method_map.find(method_name);
        if (m != m_method_map.end()) {
            return m->second.m_allow_http;
        }
        return false;
    }

    std::unordered_map<std::string, RpcMethod> m_method_map; 
};

};
