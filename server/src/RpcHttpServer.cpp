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

//FIXME:ev_handler_mt is not work for mongoose
//mongoose is not thread safe
__attribute__((unused)) static int ev_handler_mt(struct mg_connection *conn, enum mg_event ev) {
    switch (ev) {
        case MG_AUTH: return MG_TRUE;
        case MG_REQUEST:
        {
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
void sendHttpResp(mg_connection *conn, int status, const std::string &resp) {
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
                            sendHttpResp(conn, 200, resp_data);
                            break;
                        case RpcStatus::RPC_METHOD_NOTFOUND:
                            delete rpcbroker;
                            printf("[WARNING]Unknow method request #%s#\n", method_name.c_str());
                            sendHttpResp(conn, 404, std::string("Unknow method request:") + method_name);
                            break;
                        case RpcStatus::RPC_SERVER_FAIL:
                            delete rpcbroker;
                            printf("[WARNING]method call fail #%s#\n", method_name.c_str());
                            sendHttpResp(conn, 200, resp_data);
                            break;
                        default:
                            break;
                    }
                }
                else {
                    printf("[WARNING]Unknow service request #%s#\n", service_name.c_str());
                    sendHttpResp(conn, 404, std::string("Unknow service request:") + service_name);
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

void* process_proc(void* p_server)  
{  
    if (p_server == NULL) {
        printf("[ERROR]process_proc p_server NULL\n");
        return NULL;
    }
    mgthread_parameter *mptr = static_cast<mgthread_parameter *>(p_server);
    struct mg_server *mserver = static_cast<struct mg_server*>(mptr->mgserver);
    RpcHttpServer *http_server = static_cast<RpcHttpServer *>(mptr->httpserver);
    while(!http_server->isStop())
    {  
        mg_poll_server(mserver, 50);  
    }  
    return NULL;  
}  

RpcHttpServer::RpcHttpServer(RpcServerConfig &cfg, RpcServerImpl *server)
: m_server(server)
, m_stop(false)
, m_listen_port(cfg.getHttpPort())
, m_thread_num(cfg.m_http_thread_num)
, m_servers(NULL)
{
    m_servers = new mgthread_parameter[m_thread_num];
}

RpcHttpServer::~RpcHttpServer() {
    delete [] m_servers;
}

void RpcHttpServer::start() {
    for(int i = 0; i < m_thread_num; i++)  
    {  
        m_servers[i].mgserver = mg_create_server(m_server, ev_handler);  
        m_servers[i].httpserver = this;
        if(i == 0)  
        {  
            mg_set_option(m_servers[i].mgserver, "listening_port", std::to_string(m_listen_port).c_str());
        }  
        else  
        {  
            mg_copy_listeners(m_servers[0].mgserver, m_servers[i].mgserver);
        }  
        mg_start_thread(process_proc, &m_servers[i]);  
    }  
    printf("Listening on HTTP port %d\n", m_listen_port);
}


void RpcHttpServer::stop() {
    m_stop = true;
    for(int i = 0; i < m_thread_num; i++)  
    {  
        mg_destroy_server(&(m_servers[i].mgserver));
    }
}

bool RpcHttpServer::isStop() {
    return m_stop;
}

};
