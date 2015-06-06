/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>

#include "RpcWorker.h"
#include "RpcRespBroker.h"
#include "RpcConnection.h"
#include "RpcServer.h"
#include "rpc.pb.h"

namespace rpcframe
{


RpcWorker::RpcWorker(ReqQueue *workqueue, RpcServer *server)
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
    while(1) {
        if (m_stop) {
            break;
        }
        request_pkg *pkg = NULL;
        if (m_work_q->pop(pkg, 10)) {
            std::unique_ptr<request_pkg> u_ptr(pkg);
            //must get request id from here
            RpcInnerReq req;
            req.ParseFromArray(pkg->data, pkg->data_len);

            std::string resp_data;
            IService *p_service = m_server->getService(req.service_name());
            if (NULL != p_service) {
                RpcRespBroker *rpcbroker = new RpcRespBroker(m_server, pkg->connection_id, req.request_id(),
                                                            (req.type() == RpcInnerReq::TWO_WAY));
                IService::ServiceRET ret = p_service->runService(req.methond_name(), req.data(), resp_data, rpcbroker);
                if (ret == IService::ServiceRET::S_OK) {
                    delete rpcbroker;
                    if(req.type() == RpcInnerReq::TWO_WAY) {
                        //have response to send
                        RpcInnerResp resp;
                        resp.set_request_id(req.request_id());
                        resp.set_data(resp_data);
                        std::string *out_data = new std::string();
                        resp.SerializeToString(out_data);

                        rpcframe::response_pkg *resp_pkg = new rpcframe::response_pkg();
                        resp_pkg->data = out_data;
                        resp_pkg->data_len = out_data->length();

                        //put response to connection queue, max worker throughput
                        m_server->pushResp(pkg->connection_id, resp_pkg);
                    }
                }
            }

        } 
        else {
            //printf("thread: %lu, no data\n", std::this_thread::get_id());
        }
    }
}

};
