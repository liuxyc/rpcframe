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

/*std::move VS RVO ...*/
#define RPC_ADD_METHOD(class_name, method_name) \
  m_method_map.emplace(#method_name, rpcframe::RpcMethod(std::bind(&class_name::method_name, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))); 

class IService
{
  public:
    IService() {};
    virtual ~IService() {};

    RpcStatus runMethod(const std::string &method_name, 
        const std::string &req_data, 
        std::string &resp_data, 
        IRpcRespBrokerPtr resp_broker, RpcMethodStatusPtr &method_status);

    std::map<std::string, RpcMethod> m_method_map; 
};

};
#endif
