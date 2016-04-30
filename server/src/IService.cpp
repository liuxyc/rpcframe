/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include <map>
#include <string>
#include <vector>

#include "IService.h"
#include "IRpcRespBroker.h"

namespace rpcframe {
    
RpcStatus IService::runMethod(const std::string &method_name, 
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

};
