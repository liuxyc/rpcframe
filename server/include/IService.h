/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_ISERVICE
#define RPCFRAME_ISERVICE
#include <map>
#include <string>
#include <vector>

#include "IRpcRespBroker.h"
#include "RpcDefs.h"

namespace rpcframe {

#define RPC_ADD_METHOD(class_name, method_name) m_method_map[#method_name] = std::bind(&class_name::method_name, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

class IService
{
    typedef std::function<rpcframe::RpcStatus(const std::string &, std::string &, rpcframe::IRpcRespBrokerPtr)> RPC_FUNC_T;
public:
    IService() {};
    virtual ~IService() {};
    
    rpcframe::RpcStatus runMethod(const std::string &method_name, 
                          const std::string &req_data, 
                          std::string &resp_data, 
                          rpcframe::IRpcRespBrokerPtr resp_broker) 
    { 
        if (m_method_map.find(method_name) != m_method_map.end()) { 
            RPC_FUNC_T p_fun = m_method_map[method_name]; 
            return p_fun(req_data, resp_data, resp_broker); 
        }  
        else { 
            return rpcframe::RpcStatus::RPC_METHOD_NOTFOUND;  
        }  
    }; 

    void getMethodNames(std::vector<std::string> &smap) { 
        for(auto mmap: m_method_map) { 
            smap.push_back(mmap.first); 
     
        };
    };
    std::map<std::string, RPC_FUNC_T> m_method_map; 

};

};
#endif
