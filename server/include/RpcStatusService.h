/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCSTATUSSERVICE
#define RPCFRAME_RPCSTATUSSERVICE
#include <string>

#include "IService.h"
#include "RpcServerImpl.h"
#include "RpcServerConn.h"

namespace rpcframe {


class RpcServerImpl;

/**
 * @brief RpcStatusService help server send the response in async
 */
class RpcStatusService: public IService
{
public:
    explicit RpcStatusService(RpcServerImpl *server) 
    : m_rpc_server(server) {
        RPC_ADD_METHOD(RpcStatusService, get_status);
    };
    
    RpcStatus get_status(const std::string &request_data, std::string &resp_data, IRpcRespBrokerPtr resp_broker) {
      if (resp_broker->isFromHttp()) {
        resp_data = "<html><body><h1>Running:" + std::to_string(!m_rpc_server->m_stop) + "</h1>";
        resp_data += "<h1>Worker Num:" + std::to_string(m_rpc_server->m_worker_vec.size()) + "</h1>";
        resp_data += "<h1>Max conn limit:" + std::to_string(m_rpc_server->m_cfg.m_max_conn_num) + "</h1>";
        for(auto srv: m_rpc_server->m_service_map) {
          resp_data += "service name:" + srv.first + "</br>";
          std::vector<std::string> mnames;
          srv.second->getMethodNames(mnames);
          for (auto method: mnames) {
            resp_data += "&nbsp;&nbsp;&nbsp;&nbsp;method name:" + method + "</br>";
          }
        }
        resp_data += "</br>";
        for(auto conn: m_rpc_server->m_conn_set) {
          resp_data += "conn id:" + conn.first + " fd:" + std::to_string(conn.second->getFd()) + "</br>";
        }
        resp_data += "<h1>Seqid:" + std::to_string(m_rpc_server->m_seqid) + "</h1>";
        resp_data += "<h1>Req Q size:" + std::to_string(m_rpc_server->m_request_q.size()) + "</h1>";
        resp_data += "<h1>Resp Q size:" + std::to_string(m_rpc_server->m_response_q.size()) + "</h1>";
        resp_data += "<h1>Total Req num:" + std::to_string(m_rpc_server->total_req_num) + "</h1>";
        resp_data += "<h1>Total Resp num:" + std::to_string(m_rpc_server->total_resp_num) + "</h1>";
        resp_data += "<h1>Total Call num:" + std::to_string(m_rpc_server->total_call_num) + "</h1>";
        resp_data += "<h1>Avg Req wait time:" + std::to_string(m_rpc_server->avg_req_wait_time) + "ms</h1>";
        resp_data += "<h1>Avg Resp wait time:" + std::to_string(m_rpc_server->avg_resp_wait_time) + "ms</h1>";
        resp_data += "<h1>Avg Call time:" + std::to_string(m_rpc_server->avg_call_time) + "ms</h1>";
        resp_data += "<h1>Max Call time:" + std::to_string(m_rpc_server->max_call_time) + "ms</h1>";
        resp_data += "<h1>epoll fd:" + std::to_string(m_rpc_server->m_epoll_fd) + "</h1>";
        resp_data += "<h1>listening fd:" + std::to_string(m_rpc_server->m_listen_socket) + "</h1>";
        resp_data += "</body></html>";
      }
      else {
        //TODO: JSON?
      }
      return rpcframe::RpcStatus::RPC_SERVER_OK;
    };

    RpcStatusService(const RpcStatusService &) = delete;
    RpcStatusService &operator=(const RpcStatusService &) = delete;
private:
    RpcServerImpl *m_rpc_server;
};

};
#endif
