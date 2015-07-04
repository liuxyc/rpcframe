/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCWORKER
#define RPCFRAME_RPCWORKER
#include <atomic>

#include "Queue.h"
#include "IService.h"
#include "RpcPackage.h"

namespace rpcframe
{

class RpcServerImpl;

class RpcWorker
{
public:
    RpcWorker(ReqQueue *workqueue, RpcServerImpl *server);
    ~RpcWorker();
    void stop();
    void run();
private:
    ReqQueue *m_work_q;
    RpcServerImpl *m_server;
    std::atomic<bool> m_stop;
    
};

};
#endif
