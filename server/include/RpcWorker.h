/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#pragma once
#include <atomic>
#include <thread>

#include "Queue.h"
#include "RpcPackage.h"

struct mg_connection;
namespace rpcframe
{

class RpcServerImpl;

class RpcWorker
{
public:
    RpcWorker(ReqQueue *workqueue, RpcServerImpl *server);
    RpcWorker &operator=(const RpcWorker &worker) = delete;
    RpcWorker(const RpcWorker &worker) = delete;
    ~RpcWorker();
    void stop();
    void run();
private:
    ReqQueue *m_work_q;
    RpcServerImpl *m_server;
    std::atomic<bool> m_stop;
    std::thread *m_thread;
    
};

};
