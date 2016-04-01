/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include <map>
#include <string>
#include <vector>

#include "IService.h"

namespace rpcframe {
    
rpcframe::RpcStatus IService::runMethod(const std::string &method_name, 
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
}

void IService::getMethodNames(std::vector<std::string> &smap) { 
  for(auto mmap: m_method_map) { 
    smap.push_back(mmap.first); 

  };
}

};
