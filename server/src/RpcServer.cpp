/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include "RpcServer.h"

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>


namespace rpcframe
{

RpcServer::RpcServer(RpcServerConfig &cfg)
{
    m_server_impl = new RpcServerImpl(cfg);
}

RpcServer::~RpcServer() {
    delete m_server_impl;
}

bool RpcServer::start() {
    return m_server_impl->start();
}


void RpcServer::stop() {
    m_server_impl->stop();
}

};
