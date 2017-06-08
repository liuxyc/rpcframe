/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include "RpcClientWorker.h"

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>
#include <sys/prctl.h>
#include <signal.h>
#include <memory>
#include <sstream>
#include <arpa/inet.h>

#include "RpcClientConn.h"
#include "RpcDefs.h"
#include "RpcClient.h"
#include "RawDataImpl.h"
#include "rpc.pb.h"
#include "util.h"

namespace rpcframe
{


RpcClientWorker::RpcClientWorker(RpcEventLooper *ev)
: m_ev(ev)
, m_stop(false)
{

}

RpcClientWorker::~RpcClientWorker() {

}

void RpcClientWorker::stop() {
    m_stop = true;
}

void RpcClientWorker::run(RespPkgPtr pkg) {
    uint32_t proto_len = ntohl(*((uint32_t *)pkg->data));

    //must get request id from here
    RpcInnerResp resp;
    if(!resp.ParseFromArray(pkg->data + sizeof(proto_len), proto_len)) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "parse internal pkg fail");
        return;
    }
    char *resp_data = pkg->data + sizeof(proto_len) + proto_len;
    uint32_t ret_val = ntohl(*((uint32_t *)(pkg->data + (pkg->data_len - sizeof(uint32_t)) )));
    std::shared_ptr<RpcClientCallBack> cb = m_ev->getCb(resp.request_id());
    if (cb != nullptr) {
        RawData rd(resp_data, pkg->data_len - sizeof(ret_val) - sizeof(proto_len) - proto_len);
        rd.m_impl->m_pkg_keeper = pkg;
        //if marked as timeout, the callback already called by RpcCBStatus::RPC_TIMEOUT
        if (!cb->isTimeout()) {
            cb->callback_safe(static_cast<RpcStatus>(ret_val), rd);
        }
        m_ev->removeCb(resp.request_id());
    }
    else {
        //RPC_LOG(RPC_LOG_LEV::ERROR, "the cb of req:%s is nullptr", resp.request_id().c_str());
    }

}

};
