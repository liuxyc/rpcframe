/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include <memory>
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>
#include <sys/prctl.h>

#include "RpcClientWorker.h"
#include "RpcClientConn.h"
#include "RpcDefs.h"
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
    prctl(PR_SET_NAME, "RpcClientWorker", 0, 0, 0); 
    while(1) {
        if (m_stop) {
            break;
        }
        response_pkg *pkg = NULL;
        if (m_ev->m_response_q.pop(pkg, 1000)) {
            std::unique_ptr<response_pkg> u_ptr(pkg);

            //must get request id from here
            RpcInnerResp resp;
            resp.ParseFromArray(pkg->data, pkg->data_len);
            RpcClientCallBack *cb = m_ev->getCb(resp.request_id());
            if (cb != NULL) {
                std::string cb_type = cb->getType();
                //if marked as timeout, the callback already called by RpcCBStatus::RPC_TIMEOUT
                if (!cb->isTimeout()) {
                    cb->callback(static_cast<RpcStatus>(resp.ret_val()), resp.data());
                }
                else {
                    cb->callback(RpcStatus::RPC_CB_TIMEOUT, resp.data());
                }

                //NOTE:if the callback is from blocker, we do not removeCb here, the RpcClient will 
                //send another fake response and set the callback type to "timeout", at that time 
                //we can call removeCb
                if ( cb_type != "blocker" ) {
                    m_ev->removeCb(resp.request_id());
                }
            }
            else {
                //printf("the cb of req:%s is NULL\n", resp.request_id().c_str());
            }

        } 
    }
}

};
