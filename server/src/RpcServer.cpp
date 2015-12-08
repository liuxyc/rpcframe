/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include "RpcServer.h"

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>

#include "RpcServerImpl.h"

namespace rpcframe
{

RpcServer::RpcServer(RpcServerConfig &cfg)
{
    m_server_impl = new RpcServerImpl(cfg);
}

RpcServer::~RpcServer() {
    delete m_server_impl;
}

bool RpcServer::addService(const std::string &name, IService *p_service)
{
    return m_server_impl->addService(name, p_service);
}

IService *RpcServer::getService(const std::string &name)
{
    return m_server_impl->getService(name);
}

bool RpcServer::start() {
    return m_server_impl->start();
}


void RpcServer::stop() {
    m_server_impl->stop();
}

};
