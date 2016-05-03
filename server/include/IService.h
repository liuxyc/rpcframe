/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include <map>
#include <string>
#include <vector>

#include "RpcDefs.h"
#include "IRpcRespBroker.h"

namespace rpcframe {

class IServiceImpl;

/*std::move VS RVO ...*/
#define RPC_ADD_METHOD(class_name, method_name) \
  add_method(#method_name, std::bind(&class_name::method_name, this, std::placeholders::_1, std::placeholders::_2)); 

class IService
{
  public:
    IService();
    virtual ~IService();
    void add_method(const std::string &method_name, const RPC_FUNC_T func);
    IServiceImpl *m_impl;
};

};
