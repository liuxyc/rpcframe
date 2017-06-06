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
     * @param       Service instance, 
     *              if nullptr, will create instance T for each work thread
     *              not nullptr, all thread will share the instance which you give.
     *
     * @return 
     */
    template <typename T>
    bool addService(const std::string &name, IService *p_service)
    {
        if(p_service == nullptr) {
            p_service = new T();
        }
        return addServiceImp(name, p_service);
    }

    /**
     * @brief start RpcServer, this method will block
     *
     * @return 
     */
    bool start();
    void stop();

    RpcServer(const RpcServer &) = delete;
    RpcServer &operator=(const RpcServer &) = delete;
    bool addServiceImp(const std::string &name, IService *p_service);

private:
    RpcServerImpl *m_server_impl;
};

};
