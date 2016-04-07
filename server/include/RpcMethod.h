/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include <utility>
#include <string>
#include <cstdint>

#include "RpcDefs.h"
#include "SpinLock.h"

namespace rpcframe
{

class RpcMethodStatus 
{
  public:
    RpcMethodStatus();

    RpcMethodStatus(const RpcMethodStatus &) = delete;
    const RpcMethodStatus& operator=(const RpcMethodStatus &) = delete;

    void calcCallTime(uint64_t call_time);

    uint64_t total_call_nums;
    uint64_t timeout_call_nums;
    uint64_t avg_call_time;  // unit: ms
    uint64_t longest_call_time;   // unit: ms
    uint64_t call_from_http_num;
    SpinLock m_stat_lock;
};

class RpcMethod {
  public:
    explicit RpcMethod(const RPC_FUNC_T &func);
    ~RpcMethod();
    RpcMethod(RpcMethod &&m);
    RpcMethod(const RpcMethod &m) = delete;
    const RpcMethod &operator=(const RpcMethod &m) = delete;
    RPC_FUNC_T m_func;
    RpcMethodStatus *m_status;
};

typedef RpcMethodStatus* RpcMethodStatusPtr;

};
