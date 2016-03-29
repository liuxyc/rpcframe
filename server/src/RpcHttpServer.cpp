/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include "RpcHttpServer.h"

#include <sstream>
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>
#include <sys/prctl.h>

#include "IService.h"
#include "RpcRespBroker.h"
#include "RpcServerImpl.h"
#include "RpcPackage.h"
#include "rpc.pb.h"
#include "util.h"

namespace rpcframe
{

//below is the right mongoose solution, it only can deal one http request in one time
void sendHttpResp(mg_connection *conn, int status, const std::string &resp) {
    mg_printf(conn, "HTTP/1.1 %d\r\n", status);
    mg_printf(conn, "%s", "Content-Type: text/html\r\n");
    mg_printf(conn, "Content-Length: %lu\r\n", resp.size());
    mg_printf(conn, "%s", "Connection: close\r\n");
    mg_printf(conn, "%s", "\r\n");
    mg_send(conn, resp.c_str(), resp.size());
}

static void ev_handler(struct mg_connection *conn, int ev, void *ev_data) {
      struct http_message *hm = (struct http_message *) ev_data;
      switch (ev) {
        case MG_EV_HTTP_REQUEST:
        {
            std::string url_string(hm->uri.p, hm->uri.len);
            if (url_string[0] != '/') {
                mg_printf(conn, "Invalid Request URI [%s]", url_string.c_str());  
                mg_send_head(conn, 500, 0, NULL);
            }
            RpcServerImpl *server = (RpcServerImpl *)conn->user_data;
            std::string::size_type service_pos = url_string.find('/', 1);
            if (service_pos != std::string::npos) {
                std::string service_name = url_string.substr(1, service_pos - 1);
                std::string method_name = url_string.substr(service_pos + 1, url_string.size());
                IService *p_service = server->getService(service_name);
                if (p_service != nullptr) {
                    IRpcRespBrokerPtr rpcbroker = std::make_shared<RpcRespBroker>(server, "http_connection", "http_request",true, conn);
                    std::string req_data(hm->body.p, hm->body.len);
                    std::string resp_data;
                    RpcStatus ret = p_service->runMethod(method_name, req_data, resp_data, rpcbroker);
                    switch (ret) {
                        case RpcStatus::RPC_SERVER_OK:
                            sendHttpResp(conn, 200, resp_data);
                            break;
                        case RpcStatus::RPC_METHOD_NOTFOUND:
                            RPC_LOG(RPC_LOG_LEV::WARNING, "Unknow method request #%s#", method_name.c_str());
                            sendHttpResp(conn, 404, std::string("Unknow method request:") + method_name);
                            break;
                        case RpcStatus::RPC_SERVER_FAIL:
                            RPC_LOG(RPC_LOG_LEV::WARNING, "method call fail #%s#", method_name.c_str());
                            sendHttpResp(conn, 200, resp_data);
                            break;
                        //FIXME:mongoose is not thread safe, async response is not avaliabe in http mode
                        /*
                        case RpcStatus::RPC_SERVER_NONE:
                            return MG_MORE;
                            break;
                        */
                        default:
                            break;
                    }
                }
                else {
                    RPC_LOG(RPC_LOG_LEV::WARNING, "Unknow service request #%s#", service_name.c_str());
                    sendHttpResp(conn, 404, std::string("Unknow service request:") + service_name);
                }
            }
            else {
                mg_printf(conn, "Invalid Request URI [%s]", url_string.c_str());  
            }
        }
        default: 
            break;

    }
}

void* process_proc(void* p_server)  
{  
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
    if (p_server == nullptr) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "process_proc p_server nullptr");
        return nullptr;
    }
    prctl(PR_SET_NAME, "RpcHttpServer", 0, 0, 0); 
    mgthread_parameter *mptr = static_cast<mgthread_parameter *>(p_server);
    struct mg_mgr *mgr = static_cast<struct mg_mgr *>(mptr->mgserver_mgr);
    RpcHttpServer *http_server = static_cast<RpcHttpServer *>(mptr->httpserver);
    while(!http_server->isStop())
    {  
        mg_mgr_poll(mgr, 5);
    }  
    mg_mgr_free(mgr);
    return nullptr;  
}  

RpcHttpServer::RpcHttpServer(RpcServerConfig &cfg, RpcServerImpl *server)
: m_server(server)
, m_stop(false)
, m_listen_port(cfg.getHttpPort())
, m_thread_num(cfg.m_http_thread_num)
{
    memset(&m_mgr, 0, sizeof(m_mgr));
}

RpcHttpServer::~RpcHttpServer() {
}

void RpcHttpServer::start() {
  struct mg_connection *nc;

  mg_mgr_init(&m_mgr, NULL);
  nc = mg_bind(&m_mgr, std::to_string(m_listen_port).c_str(), ev_handler);
  mg_set_protocol_http_websocket(nc);

  /* For each new connection, execute ev_handler in a separate thread */
  mg_enable_multithreading(nc);

  m_mgserver.httpserver = this;
  m_mgserver.mgserver_mgr = &m_mgr;
  nc->user_data = m_server;
  mg_start_thread(process_proc, &m_mgserver);  
  RPC_LOG(RPC_LOG_LEV::INFO, "Listening on HTTP port %d", m_listen_port);
}


void RpcHttpServer::stop() {
    m_stop = true;
    RPC_LOG(RPC_LOG_LEV::INFO, "wait http thread stop for 2 second...");
    sleep(2);
}

bool RpcHttpServer::isStop() {
    return m_stop;
}

};
