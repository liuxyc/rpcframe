/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_ISERVICE
#define RPCFRAME_ISERVICE
#include <map>
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

    void add_method(const std::string &method_name, const RpcMethod &method) {
      m_method_map.emplace(method_name, method);
    }

    std::map<std::string, RpcMethod> m_method_map; 
};

};
#endif
