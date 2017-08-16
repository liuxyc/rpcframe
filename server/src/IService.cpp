/*
 * Copyright (c) 2015-2017, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include <map>
#include <string>
#include <vector>

#include "IService.h"
#include "IServiceImpl.h"

namespace rpcframe {
    
  IService::IService() 
  {
    m_impl = new IServiceImpl();
  }

  IService::~IService() 
  {
    delete m_impl;
  }
  void IService::add_method(const std::string &method_name, const RPC_FUNC_T &func, bool allow_http)
  {
    m_impl->add_method(method_name, func, allow_http);
  }

};
