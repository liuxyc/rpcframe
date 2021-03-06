/*
 * Copyright (c) 2015-2017, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#pragma once
#include "RpcPackage.h"

namespace rpcframe
{
  class RawDataImpl 
  {
    public:
      RawDataImpl()
      : m_pkg_keeper(new response_pkg(1))
      {
      }
      RawDataImpl(RawDataImpl &r)
      {
        m_pkg_keeper.swap(r.m_pkg_keeper);
      }
      RawDataImpl &operator=(RawDataImpl &r)
      {
        m_pkg_keeper.swap(r.m_pkg_keeper);
        return *this;
      }
      ~RawDataImpl()
      {}

      RespPkgPtr m_pkg_keeper;
  };

};
