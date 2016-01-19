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
            //must get request id from here
            RpcInnerReq req;
            if (!req.ParseFromArray(pkg->data, pkg->data_len)) {
                RPC_LOG(RPC_LOG_LEV::ERROR, "parse internal pkg fail");
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
                IRpcRespBrokerPtr rpcbroker = std::make_shared<RpcRespBroker>(m_server, pkg->connection_id, req.request_id(),
                                                            (req.type() == RpcInnerReq::TWO_WAY), nullptr);

                RpcStatus ret = p_service->runService(req.method_name(), 
                                                                 req.data(), 
                                                                 resp_data, 
                                                                 rpcbroker);
                resp.set_ret_val(static_cast<uint32_t>(ret));
                switch (ret) {
                    case RpcStatus::RPC_SERVER_OK:
                        if(req.type() == RpcInnerReq::HTTP) {
                            mg_connection *conn = (struct mg_connection *)pkg->http_conn;
                            sendHttpOk(conn, resp_data);
                        }
                        break;
                    case RpcStatus::RPC_METHOD_NOTFOUND:
                        RPC_LOG(RPC_LOG_LEV::WARNING, "Unknow method request #%s#", req.method_name().c_str());
                        if(req.type() == RpcInnerReq::HTTP) {
                            mg_connection *conn = (struct mg_connection *)pkg->http_conn;
                            sendHttpFail(conn, 404, std::string("Unknow method request:") + req.method_name());
                        }
                        break;
                    case RpcStatus::RPC_SERVER_FAIL:
                        RPC_LOG(RPC_LOG_LEV::WARNING, "method call fail #%s#", req.method_name().c_str());
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
                RPC_LOG(RPC_LOG_LEV::WARNING, "Unknow service request #%s#", req.service_name().c_str());
                if(req.type() == RpcInnerReq::HTTP) {
                    mg_connection *conn = (struct mg_connection *)pkg->http_conn;
                    sendHttpFail(conn, 404, std::string("Unknow service request:") + req.service_name());
                }
            }
            if (req.type() == RpcInnerReq::TWO_WAY) {
                resp.set_data(resp_data);
                //NOTICE: memory fragments
                RespPkgPtr resp_pkg = RespPkgPtr(new response_pkg(resp.ByteSize()));
                if(!resp.SerializeToArray(resp_pkg->data, resp_pkg->data_len)) {
                    RPC_LOG(RPC_LOG_LEV::ERROR, "serialize innernal pkg fail");
                }
                else {
                    //put response to connection queue, max worker throughput
                    m_server->pushResp(pkg->connection_id, resp_pkg);
                }
            }
        } 
        else {
            //RPC_LOG(RPC_LOG_LEV::DEBUG, "thread: %lu, no data", std::this_thread::get_id());
        }
    }
}

//FIXME:this code is not work!
//mongoose is not thread safe, deal http request is not avaliable in RpcWorker
void RpcWorker::sendHttpOk(mg_connection *conn, const std::string &resp) {
    mg_printf(conn, "HTTP/1.1 %d\r\n", 200);
    mg_printf(conn, "%s", "Content-Type: text/html\r\n");
    mg_printf(conn, "Content-Length: %lu\r\n", resp.size());
    mg_printf(conn, "%s", "Connection: close\r\n");
    mg_printf(conn, "%s", "\r\n");
    mg_send(conn, resp.c_str(), resp.size());
}

//FIXME:this code is not work!
//mongoose is not thread safe, deal http request is not avaliable in RpcWorker
void RpcWorker::sendHttpFail(mg_connection *conn, int status, const std::string &resp) {
    mg_printf(conn, "HTTP/1.1 %d\r\n", status);
    mg_printf(conn, "%s", "Content-Type: text/html\r\n");
    mg_printf(conn, "Content-Length: %lu\r\n", resp.size());
    mg_printf(conn, "%s", "Connection: close\r\n");
    mg_printf(conn, "%s", "\r\n");
    mg_send(conn, resp.c_str(), resp.size());
}

};
