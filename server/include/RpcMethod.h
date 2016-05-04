/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include <utility>
#include <string>
#include <cstdint>

#include "RpcDefs.h"
#include "IService.h"

namespace rpcframe
{
class SpinLock;

class RpcMethodStatus 
{
  public:
    RpcMethodStatus();
    ~RpcMethodStatus();
    RpcMethodStatus(const RpcMethodStatus &) = delete;
    const RpcMethodStatus& operator=(const RpcMethodStatus &) = delete;

    void calcCallTime(uint64_t call_time);

    bool enabled;
    uint64_t total_call_nums;
    uint64_t timeout_call_nums;
    uint64_t avg_call_time;  // unit: ms
    uint64_t longest_call_time;   // unit: ms
    uint64_t call_from_http_num;
    SpinLock *m_stat_lock;
};

typedef RpcMethodStatus* RpcMethodStatusPtr;

class RpcMethod {
  public:
    explicit RpcMethod(const RPC_FUNC_T &func);
    ~RpcMethod();
    RpcMethod(RpcMethod &&m);
    RpcMethod(const RpcMethod &m);
    const RpcMethod &operator=(const RpcMethod &m) = delete;
    RPC_FUNC_T m_func;
    RpcMethodStatusPtr m_status;
};
};
