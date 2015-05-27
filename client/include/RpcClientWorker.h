/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCCLIENTWORKER
#define RPCFRAME_RPCCLIENTWORKER
#include <utility>
#include <unordered_map>
#include <map>
#include <vector>
#include <atomic>
#include "Queue.h"

namespace rpcframe
{

class RpcEventLooper;

class RpcClientWorker
{
public:
    RpcClientWorker(RpcEventLooper *ev);
    ~RpcClientWorker();
    void stop();
    void run();
private:
    RpcEventLooper *m_ev;
    std::atomic<bool> m_stop;
    
};

};
#endif
