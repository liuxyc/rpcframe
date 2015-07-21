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
    void sendHttpOk(mg_connection *conn, const std::string &resp);
    void sendHttpFail(mg_connection *conn, int status, const std::string &resp);

    RpcHttpServer(const RpcHttpServer &) = delete;
    RpcHttpServer &operator=(const RpcHttpServer &) = delete;

private:
    RpcServerImpl *m_server;
    std::atomic<bool> m_stop;
    int m_listen_port;
};

};
#endif
