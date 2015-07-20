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

struct mg_connection;
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
    void sendHttpOk(mg_connection *conn, const std::string &resp);
    void sendHttpFail(mg_connection *conn, const std::string &resp);

    ReqQueue *m_work_q;
    RpcServerImpl *m_server;
    std::atomic<bool> m_stop;
    
};

};
#endif
