/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCDEFS
#define RPCFRAME_RPCDEFS
#include <utility>
#include <string>
#include <functional>
#include <memory>

namespace rpcframe
{

    enum class RpcStatus {
        RPC_SEND_OK = 0,
        RPC_SEND_FAIL,
        RPC_REQ_TOO_LARGE,
        RPC_DISCONNECTED,
        RPC_SEND_TIMEOUT,
        RPC_CB_TIMEOUT,
        RPC_SRV_NOTFOUND,
        RPC_METHOD_NOTFOUND,
        RPC_SERVER_OK,
        RPC_SERVER_NONE,
        RPC_SERVER_FAIL,
        RPC_MALLOC_PKG_FAIL,
    };

    class IRpcRespBroker;

    typedef std::shared_ptr<IRpcRespBroker> IRpcRespBrokerPtr;
    typedef std::function<rpcframe::RpcStatus(const std::string &, std::string &, IRpcRespBrokerPtr)> RPC_FUNC_T;
};
#endif
