/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include "RpcDefs.h"
#include "IService.h"
#include "RpcServerConfig.h"

namespace rpcframe
{

class RpcServerImpl;
class RpcServer
{
public:
    explicit RpcServer(RpcServerConfig &cfg);
    ~RpcServer();

    /**
     * @brief add IService implement to RpcServer
     *
     * @param name  Service name
     * @param       Service instance
     *
     * @return 
     */
    bool addService(const std::string &name, IService *);
    IService *getService(const std::string &name);

    /**
     * @brief start RpcServer, this method will block
     *
     * @return 
     */
    bool start();
    void stop();

    RpcServer(const RpcServer &) = delete;
    RpcServer &operator=(const RpcServer &) = delete;

private:
    RpcServerImpl *m_server_impl;
};

};
