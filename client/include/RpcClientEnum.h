/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCCLIENTENUM
#define RPCFRAME_RPCCLIENTENUM


namespace rpcframe
{
    enum class RpcStatus {
        RPC_SEND_OK = 0,
        RPC_SEND_FAIL,
        RPC_DISCONNECTED,
        RPC_SEND_TIMEOUT,
        RPC_CB_OK,
        RPC_CB_TIMEOUT,
    };
};
#endif
