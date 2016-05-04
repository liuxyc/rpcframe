/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include "RpcDefs.h"
#include <atomic>
#include "mongoose.h"

namespace rpcframe
{

class RpcServerImpl;
class RpcServerConfig;
class RpcHttpServer;


typedef struct mg_server * mongoose_server_ptr;

class mgthread_parameter
{
public:
    struct mg_mgr *mgserver_mgr;
    RpcHttpServer *httpserver;
};

class RpcHttpServer
{
public:
    RpcHttpServer(RpcServerConfig &cfg, RpcServerImpl *);
    ~RpcHttpServer();

    void start();
    void stop();
    bool isStop();
    void sendHttpOk(mg_connection *conn, const std::string &resp);
    void sendHttpFail(mg_connection *conn, int status, const std::string &resp);

    RpcHttpServer(const RpcHttpServer &) = delete;
    RpcHttpServer &operator=(const RpcHttpServer &) = delete;

private:
    RpcServerImpl *m_server;
    std::atomic<bool> m_stop;
    int m_listen_port;
    int m_thread_num;
    mgthread_parameter m_mgserver;
    struct mg_mgr m_mgr;
};

};
