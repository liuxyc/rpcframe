/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCHTTPSERVER
#define RPCFRAME_RPCHTTPSERVER
#include "RpcDefs.h"
#include <atomic>
#include "mongoose.h"

namespace rpcframe
{

class RpcServerImpl;

class RpcHttpServer
{
public:
    RpcHttpServer(RpcServerConfig &cfg, RpcServerImpl *);
    ~RpcHttpServer();

    void start();
    void stop();

    RpcHttpServer(const RpcHttpServer &) = delete;
    RpcHttpServer &operator=(const RpcHttpServer &) = delete;

private:
    RpcServerImpl *m_server;
    std::atomic<bool> m_stop;
    int m_listen_port;
};

};
#endif
