/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include <atomic>
#include "Queue.h"

namespace rpcframe
{

class RpcEventLooper;

class RpcClientWorker
{
public:
    explicit RpcClientWorker(RpcEventLooper *ev);
    ~RpcClientWorker();
    void stop();
    void run();
private:
    RpcEventLooper *m_ev;
    std::atomic<bool> m_stop;
    
};

};
