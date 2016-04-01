/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCWORKER
#define RPCFRAME_RPCWORKER
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
#endif
