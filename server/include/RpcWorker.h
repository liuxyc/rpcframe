/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCWORKER
#define RPCFRAME_RPCWORKER
#include <utility>
#include <unordered_map>
#include <map>
#include <vector>
#include <atomic>

#include "Queue.h"
#include "IService.h"
#include "RpcPackage.h"

namespace rpcframe
{

class RpcServer;

class RpcWorker
{
public:
    RpcWorker(ReqQueue *workqueue, RpcServer *server);
    ~RpcWorker();
    void stop();
    void run();
private:
    ReqQueue *m_work_q;
    RespQueue *m_resp_q;
    RpcServer *m_server;
    std::atomic<bool> m_stop;
    
};

};
#endif
