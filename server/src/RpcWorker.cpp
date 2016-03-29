/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include "RpcWorker.h"

#include <sstream>
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>
#include <sys/prctl.h>

#include "mongoose.h"

#include "RpcRespBroker.h"
#include "RpcServerConn.h"
#include "RpcServerImpl.h"
#include "rpc.pb.h"
#include "util.h"

namespace rpcframe
{


RpcWorker::RpcWorker(ReqQueue *workqueue, RpcServerImpl *server)
: m_work_q(workqueue)
, m_server(server)
, m_stop(false)
{
    m_thread = new std::thread(&RpcWorker::run, this);
}

RpcWorker::~RpcWorker() {
  delete m_thread;
}

void RpcWorker::stop() {
    m_stop = true;
    m_thread->join();
}

void RpcWorker::run() {
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
    prctl(PR_SET_NAME, "RpcSWorker", 0, 0, 0); 
    while(1) {
        if (m_stop) {
            break;
        }
        std::shared_ptr<request_pkg> pkg = nullptr;
        if (m_work_q->pop(pkg, 10)) {
            auto during = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - pkg->gen_time);
            m_server->calcReqQTime(during.count());
            RPC_LOG(RPC_LOG_LEV::DEBUG, "req stay: %d ms", during.count());

            //must get request id from here
            RpcInnerReq req;
            if (!req.ParseFromArray(pkg->data, pkg->data_len)) {
                RPC_LOG(RPC_LOG_LEV::ERROR, "parse internal pkg fail");
                continue;
            }

            std::string resp_data;
            IService *p_service = m_server->getService(req.service_name());
            RpcInnerResp resp;
            resp.set_request_id(req.request_id());
            if (p_service != nullptr) {
                IRpcRespBrokerPtr rpcbroker = std::make_shared<RpcRespBroker>(m_server, pkg->connection_id, req.request_id(),
                                                            (req.type() == RpcInnerReq::TWO_WAY), nullptr);

                std::chrono::system_clock::time_point begin_call_timepoint = std::chrono::system_clock::now();
                RpcStatus ret = p_service->runService(req.method_name(), 
                                                                 req.data(), 
                                                                 resp_data, 
                                                                 rpcbroker);
                during = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - begin_call_timepoint);
                m_server->calcCallTime(during.count());
                RPC_LOG(RPC_LOG_LEV::DEBUG, "call take: %d ms", during.count());
                resp.set_ret_val(static_cast<uint32_t>(ret));
                switch (ret) {
                    case RpcStatus::RPC_SERVER_OK:
                        break;
                    case RpcStatus::RPC_METHOD_NOTFOUND:
                        RPC_LOG(RPC_LOG_LEV::WARNING, "Unknow method request #%s#", req.method_name().c_str());
                        break;
                    case RpcStatus::RPC_SERVER_FAIL:
                        RPC_LOG(RPC_LOG_LEV::WARNING, "method call fail #%s#", req.method_name().c_str());
                        break;
                    case RpcStatus::RPC_SERVER_NONE:
                        continue;
                        break;
                    default:
                        break;
                }
            }
            else {
                resp.set_ret_val(static_cast<uint32_t>(RpcStatus::RPC_SRV_NOTFOUND));
                RPC_LOG(RPC_LOG_LEV::WARNING, "Unknow service request #%s#", req.service_name().c_str());
            }
            if (req.type() == RpcInnerReq::TWO_WAY) {
                resp.set_data(resp_data);
                //put response to connection queue, max worker throughput
                m_server->pushResp(pkg->connection_id, resp);
            }
        } 
        else {
            //RPC_LOG(RPC_LOG_LEV::DEBUG, "thread: %lu, no data", std::this_thread::get_id());
        }
    }
}

};
