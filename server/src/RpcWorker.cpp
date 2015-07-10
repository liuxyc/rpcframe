/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>
#include <sys/prctl.h>

#include "RpcWorker.h"
#include "RpcRespBroker.h"
#include "RpcServerConn.h"
#include "RpcServerImpl.h"
#include "rpc.pb.h"

namespace rpcframe
{


RpcWorker::RpcWorker(ReqQueue *workqueue, RpcServerImpl *server)
: m_work_q(workqueue)
, m_server(server)
, m_stop(false)
{

}

RpcWorker::~RpcWorker() {

}

void RpcWorker::stop() {
    m_stop = true;
}

void RpcWorker::run() {
    prctl(PR_SET_NAME, "RpcSWorker", 0, 0, 0); 
    while(1) {
        if (m_stop) {
            break;
        }
        request_pkg *pkg = NULL;
        if (m_work_q->pop(pkg, 10)) {
            std::unique_ptr<request_pkg> u_ptr(pkg);
            //must get request id from here
            RpcInnerReq req;
            if (!req.ParseFromArray(pkg->data, pkg->data_len)) {
                printf("[ERROR]parse internal pkg fail\n");
                continue;
            }


            std::string resp_data;
            IService *p_service = m_server->getService(req.service_name());
            RpcInnerResp resp;
            resp.set_request_id(req.request_id());
            if (p_service != NULL) {
                IRpcRespBroker *rpcbroker = new RpcRespBroker(m_server, pkg->connection_id, req.request_id(),
                                                            (req.type() == RpcInnerReq::TWO_WAY));

                RpcStatus ret = p_service->runService(req.method_name(), 
                                                                 req.data(), 
                                                                 resp_data, 
                                                                 rpcbroker);
                resp.set_ret_val(static_cast<uint32_t>(ret));
                switch (ret) {
                    case RpcStatus::RPC_SERVER_OK:
                        delete rpcbroker;
                        break;
                    case RpcStatus::RPC_METHOD_NOTFOUND:
                        delete rpcbroker;
                        printf("[WARNING]Unknow method request #%s#\n", req.method_name().c_str());
                        break;
                    case RpcStatus::RPC_SERVER_FAIL:
                        delete rpcbroker;
                        printf("[WARNING]method call fail #%s#\n", req.method_name().c_str());
                        break;
                    default:
                        break;
                }
            }
            else {
                resp.set_ret_val(static_cast<uint32_t>(RpcStatus::RPC_SRV_NOTFOUND));
                printf("[WARNING]Unknow service request #%s#\n", req.service_name().c_str());
            }
            if (req.type() == RpcInnerReq::TWO_WAY) {
                resp.set_data(resp_data);
                response_pkg *resp_pkg = new response_pkg(resp.ByteSize());
                if(!resp.SerializeToArray(resp_pkg->data, resp_pkg->data_len)) {
                    delete resp_pkg;
                    printf("[ERROR]serialize innernal pkg fail\n");
                }
                else {
                    //put response to connection queue, max worker throughput
                    m_server->pushResp(pkg->connection_id, resp_pkg);
                }
            }

        } 
        else {
            //printf("thread: %lu, no data\n", std::this_thread::get_id());
        }
    }
}

};
