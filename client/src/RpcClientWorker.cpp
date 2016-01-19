/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
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

#include "RpcClientConn.h"
#include "RpcDefs.h"
#include "RpcClient.h"
#include "RpcEventLooper.h"
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

void RpcClientWorker::run() {
    sigset_t set;
    int s;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    s = pthread_sigmask(SIG_BLOCK, &set, NULL);
    std::stringstream ss;
    ss << std::this_thread::get_id();
    if (s != 0) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "thread %s block SIGINT fail", ss.str().c_str());
    }
    prctl(PR_SET_NAME, "RpcClientWorker", 0, 0, 0); 
    while(1) {
        if (m_stop) {
            break;
        }
        std::shared_ptr<response_pkg> pkg;
        if (m_ev->m_response_q.pop(pkg, 1000)) {

            //must get request id from here
            RpcInnerResp resp;
            if(!resp.ParseFromArray(pkg->data, pkg->data_len)) {
                RPC_LOG(RPC_LOG_LEV::ERROR, "[ERROR]parse internal pkg fail");
                continue;
            }
            std::shared_ptr<RpcClientCallBack> cb = m_ev->getCb(resp.request_id());
            if (cb != nullptr) {
                //if marked as timeout, the callback already called by RpcCBStatus::RPC_TIMEOUT
                if (!cb->isTimeout()) {
                    cb->callback_safe(static_cast<RpcStatus>(resp.ret_val()), resp.data());
                }
                m_ev->removeCb(resp.request_id());
            }
            else {
                //RPC_LOG(RPC_LOG_LEV::ERROR, "the cb of req:%s is nullptr", resp.request_id().c_str());
            }

        } 
    }
}

};
