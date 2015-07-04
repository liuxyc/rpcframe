/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCSERVER
#define RPCFRAME_RPCSERVER
#include "IService.h"
#include "RpcDefs.h"

class RpcServerImpl;

namespace rpcframe
{

class RpcServer
{
public:
    RpcServer(RpcServerConfig &cfg);
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

private:
    RpcServerImpl *m_server_impl;
};

};
#endif
