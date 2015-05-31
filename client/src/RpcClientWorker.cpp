/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>

#include "RpcClientWorker.h"
#include "RpcClientConn.h"
#include "RpcClientEnum.h"
#include "RpcClient.h"
#include "RpcEventLooper.h"
#include "rpc.pb.h"

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

void RpcClientWorker::run() {
    while(1) {
        if (m_stop) {
            break;
        }
        server_resp_pkg *pkg = NULL;
        if (m_ev->m_response_q.pop(pkg, 1000)) {

            //must get request id from here
            RpcInnerResp resp;
            resp.ParseFromString(std::string(pkg->data, pkg->data_len));
            RpcClientCallBack *cb = m_ev->getCb(resp.request_id());
            if (NULL != cb) {
                //if marked as timeout, the callback already called by RpcCBStatus::RPC_TIMEOUT
                if (!cb->isTimeout()) {
                    cb->callback(RpcStatus::RPC_CB_OK, resp.data());
                }
                std::string cb_type = cb->getType();
                if ( cb_type != "blocker" ) {
                    m_ev->removeCb(resp.request_id());
                }
            }
            else {
                //printf("the cb of req:%s is NULL\n", resp.request_id().c_str());
            }
            delete pkg;

        } 
    }
}

};
