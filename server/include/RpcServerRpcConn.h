/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include <mutex>
#include <memory>

#include "RpcServerConn.h"

namespace rpcframe {

typedef std::pair<int, std::shared_ptr<request_pkg> > pkg_ret_t;

class RpcServerImpl;
class EpollStruct;

class RpcServerRpcConn : public RpcServerConn
{
public:
    RpcServerRpcConn(int fd, const char *seqid, RpcServerImpl *server);
    ~RpcServerRpcConn();

    virtual pkg_ret_t getRequest() override;

private:
    bool readPkgLen(uint32_t &pkg_len);
    PkgIOStatus readPkgData();
};
};
