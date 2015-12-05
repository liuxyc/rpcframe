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
#include "mongoose.h"

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

//FIXME:any HTTP related code is not work!
//mongoose is not thread safe, deal http request is not avaliable in RpcWorker
//need find new solution to replace the mg_* code
//the new solution should support handle "conn" per thread
void RpcWorker::run() {
    prctl(PR_SET_NAME, "RpcSWorker", 0, 0, 0); 
    while(1) {
        if (m_stop) {
            break;
        }
        request_pkg *pkg = nullptr;
        if (m_work_q->pop(pkg, 10)) {
            std::unique_ptr<request_pkg> u_ptr(pkg);
            //must get request id from here
            RpcInnerReq req;
            if (!req.ParseFromArray(pkg->data, pkg->data_len)) {
                printf("[ERROR]parse internal pkg fail\n");
                if(req.type() == RpcInnerReq::HTTP) {
                    mg_connection *conn = (struct mg_connection *)pkg->http_conn;
                    sendHttpFail(conn, 500, "");
                }
                continue;
            }

            std::string resp_data;
            IService *p_service = m_server->getService(req.service_name());
            RpcInnerResp resp;
            resp.set_request_id(req.request_id());
            if (p_service != nullptr) {
                IRpcRespBroker *rpcbroker = new RpcRespBroker(m_server, pkg->connection_id, req.request_id(),
                                                            (req.type() == RpcInnerReq::TWO_WAY), nullptr);

                RpcStatus ret = p_service->runService(req.method_name(), 
                                                                 req.data(), 
                                                                 resp_data, 
                                                                 rpcbroker);
                resp.set_ret_val(static_cast<uint32_t>(ret));
                switch (ret) {
                    case RpcStatus::RPC_SERVER_OK:
                        delete rpcbroker;
                        if(req.type() == RpcInnerReq::HTTP) {
                            mg_connection *conn = (struct mg_connection *)pkg->http_conn;
                            sendHttpOk(conn, resp_data);
                        }
                        break;
                    case RpcStatus::RPC_METHOD_NOTFOUND:
                        delete rpcbroker;
                        printf("[WARNING]Unknow method request #%s#\n", req.method_name().c_str());
                        if(req.type() == RpcInnerReq::HTTP) {
                            mg_connection *conn = (struct mg_connection *)pkg->http_conn;
                            sendHttpFail(conn, 404, std::string("Unknow method request:") + req.method_name());
                        }
                        break;
                    case RpcStatus::RPC_SERVER_FAIL:
                        delete rpcbroker;
                        printf("[WARNING]method call fail #%s#\n", req.method_name().c_str());
                        if(req.type() == RpcInnerReq::HTTP) {
                            mg_connection *conn = (struct mg_connection *)pkg->http_conn;
                            sendHttpOk(conn, resp_data);
                        }
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
                printf("[WARNING]Unknow service request #%s#\n", req.service_name().c_str());
                if(req.type() == RpcInnerReq::HTTP) {
                    mg_connection *conn = (struct mg_connection *)pkg->http_conn;
                    sendHttpFail(conn, 404, std::string("Unknow service request:") + req.service_name());
                }
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

//FIXME:this code is not work!
//mongoose is not thread safe, deal http request is not avaliable in RpcWorker
void RpcWorker::sendHttpOk(mg_connection *conn, const std::string &resp) {
    mg_send_header(conn, "Content-Type", "text/plain");
    mg_send_header(conn, "Content-Length", std::to_string(resp.size()).c_str());
    mg_send_header(conn, "Connection", "close");
    mg_write(conn, "\r\n", 2);
    mg_printf(conn, resp.c_str());
}

//FIXME:this code is not work!
//mongoose is not thread safe, deal http request is not avaliable in RpcWorker
void RpcWorker::sendHttpFail(mg_connection *conn, int status, const std::string &resp) {
    mg_send_status(conn, status);
    mg_send_header(conn, "Content-Type", "text/plain");
    mg_send_header(conn, "Content-Length", std::to_string(resp.size()).c_str());
    mg_send_header(conn, "Connection", "close");
    mg_write(conn, "\r\n", 2);
    mg_printf(conn, resp.c_str());
}

};
