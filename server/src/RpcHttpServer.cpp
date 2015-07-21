/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>

#include "RpcHttpServer.h"
#include "IService.h"
#include "RpcRespBroker.h"
#include "RpcServerImpl.h"
#include "RpcPackage.h"
#include "rpc.pb.h"

namespace rpcframe
{

//FIXME:ev_handler_mt is not work!
//mongoose is not thread safe
static int ev_handler_mt(struct mg_connection *conn, enum mg_event ev) {
    switch (ev) {
        case MG_AUTH: return MG_TRUE;
        case MG_REQUEST:
        {
            printf("Income conn %p\n", conn);
            std::string url_string = conn->uri;
            if (url_string[0] != '/') {
                mg_printf_data(conn, "Invalid Request URI [%s]", conn->uri);  
                mg_send_status(conn, 500);
                return MG_TRUE;
            }
            RpcServerImpl *server = (RpcServerImpl *)conn->server_param;
            std::string::size_type service_pos = url_string.find('/', 1);
            if (service_pos != std::string::npos) {
                std::string service_name = url_string.substr(1, service_pos - 1);
                std::string method_name = url_string.substr(service_pos + 1, url_string.size());
                RpcInnerReq req;
                req.set_service_name(service_name);
                req.set_method_name(method_name);

                req.set_request_id("http_request");
                req.set_data(conn->content, conn->content_len);
                req.set_type(RpcInnerReq::HTTP);
                request_pkg *rpk = new request_pkg(req.ByteSize(), "http_connection");
                rpk->http_conn = (void *)conn;
                req.SerializeToArray(rpk->data, rpk->data_len);
                if (!server->pushReq(rpk)) {
                    delete rpk;
                    mg_printf_data(conn, "Http server error");
                    mg_send_status(conn, 500);
                    return MG_TRUE;
                }
            }
            else {
                mg_printf_data(conn, "Invalid Request URI [%s]", conn->uri);  
                return MG_FALSE;
            }
            return MG_MORE;
        }
        default: return MG_FALSE;
    }
}

//below is the right mongoose solution, it only can deal one http request in one time
void sendHttpOk(mg_connection *conn, const std::string &resp) {
    mg_send_header(conn, "Content-Type", "text/plain");
    mg_send_header(conn, "Content-Length", std::to_string(resp.size()).c_str());
    mg_send_header(conn, "Connection", "close");
    mg_write(conn, "\r\n", 2);
    mg_printf(conn, resp.c_str());
}

void sendHttpFail(mg_connection *conn, int status, const std::string &resp) {
    mg_send_status(conn, status);
    mg_send_header(conn, "Content-Type", "text/plain");
    mg_send_header(conn, "Content-Length", std::to_string(resp.size()).c_str());
    mg_send_header(conn, "Connection", "close");
    mg_write(conn, "\r\n", 2);
    mg_printf(conn, resp.c_str());
}

static int ev_handler(struct mg_connection *conn, enum mg_event ev) {
    switch (ev) {
        case MG_AUTH: return MG_TRUE;
        case MG_REQUEST:
        {
            printf("Income conn %p\n", conn);
            std::string url_string = conn->uri;
            if (url_string[0] != '/') {
                mg_printf_data(conn, "Invalid Request URI [%s]", conn->uri);  
                mg_send_status(conn, 500);
                return MG_TRUE;
            }
            RpcServerImpl *server = (RpcServerImpl *)conn->server_param;
            std::string::size_type service_pos = url_string.find('/', 1);
            if (service_pos != std::string::npos) {
                std::string service_name = url_string.substr(1, service_pos - 1);
                std::string method_name = url_string.substr(service_pos + 1, url_string.size());
                IService *p_service = server->getService(service_name);
                if (p_service != NULL) {
                    IRpcRespBroker *rpcbroker = new RpcRespBroker(server, "http_connection", "http_request",
                                                                false);
                    std::string req_data(conn->content, conn->content_len);
                    std::string resp_data;
                    RpcStatus ret = p_service->runService(method_name, 
                                                                     req_data, 
                                                                     resp_data, 
                                                                     rpcbroker);
                    switch (ret) {
                        case RpcStatus::RPC_SERVER_OK:
                            delete rpcbroker;
                            sendHttpOk(conn, resp_data);
                            break;
                        case RpcStatus::RPC_METHOD_NOTFOUND:
                            delete rpcbroker;
                            printf("[WARNING]Unknow method request #%s#\n", method_name.c_str());
                            sendHttpFail(conn, 404, std::string("Unknow method request:") + method_name);
                            break;
                        case RpcStatus::RPC_SERVER_FAIL:
                            delete rpcbroker;
                            printf("[WARNING]method call fail #%s#\n", method_name.c_str());
                            sendHttpOk(conn, resp_data);
                            break;
                        default:
                            break;
                    }
                }
                else {
                    printf("[WARNING]Unknow service request #%s#\n", service_name.c_str());
                    sendHttpFail(conn, 404, std::string("Unknow service request:") + service_name);
                }
            }
            else {
                mg_printf_data(conn, "Invalid Request URI [%s]", conn->uri);  
                return MG_FALSE;
            }
        }
        default: return MG_FALSE;
    }
}


RpcHttpServer::RpcHttpServer(RpcServerConfig &cfg, RpcServerImpl *server)
: m_server(server)
, m_stop(false)
, m_listen_port(cfg.getHttpPort())
{
}

RpcHttpServer::~RpcHttpServer() {
}

void RpcHttpServer::start() {
  struct mg_server *server;

  // Create and configure the server
  server = mg_create_server(m_server, ev_handler);
  mg_set_option(server, "listening_port", std::to_string(m_listen_port).c_str());

  // Serve request. Hit Ctrl-C to terminate the program
  printf("Starting on port %s\n", mg_get_option(server, "listening_port"));
  while(!m_stop) {
    mg_poll_server(server, 1);
  }

  // Cleanup, and free server instance
  mg_destroy_server(&server);
}


void RpcHttpServer::stop() {
    m_stop = true;
}

};
