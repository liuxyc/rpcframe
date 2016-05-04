/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include <utility>
#include <string>
#include <functional>
#include <memory>
#include "RpcDefs.h"
#include "RawDataImpl.h"

namespace rpcframe
{
RawData::RawData()
  : RawData(nullptr, 0)
{}

RawData::RawData(const std::string &s)
: RawData((char *)s.data(), s.size())
{}

RawData::RawData(const char *d, size_t l)
: data((char *)d)
, data_len(l)
{
  m_impl = new RawDataImpl();
}

RawData::RawData(const RawData& r)
{
  data = r.data;
  data_len = r.data_len;
  *m_impl = *(r.m_impl);
}

RawData &RawData::operator=(const RawData& r)
{
  data = r.data;
  data_len = r.data_len;
  *m_impl = *(r.m_impl);
  return *this;
}

RawData::~RawData()
{
  delete m_impl;
}

size_t RawData::size() const
{
  return data_len;
}

};
