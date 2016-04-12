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
#include "RpcServerConfig.h"
#include "IRpcRespBroker.h"
#include <unistd.h>

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
        //not static get_status call, hack m_status to nullptr
        auto st = m_method_map.find("get_status");
        if(st != m_method_map.end()) {
          st->second.m_status->enabled = false;
        }
    };
    
    RpcStatus get_status(const std::string &request_data, std::string &resp_data, IRpcRespBrokerPtr resp_broker) {
      if (resp_broker->isFromHttp()) {
        resp_data = "<html><body><h1>Server Status:<h1>";
        resp_data += "<h3>Pid:" + std::to_string(getpid()) + "</h3>";
        resp_data += "<h3>Running:" + std::to_string(!m_rpc_server->m_stop) + "</h3>";
        resp_data += "<h3>Worker num:" + std::to_string(m_rpc_server->m_worker_vec.size()) + "</h3>";
        resp_data += "<h3>Max conn limit:" + std::to_string(m_rpc_server->m_cfg.m_max_conn_num) + "</h3>";
        resp_data += "<h3>Rejected conn num:" + std::to_string(m_rpc_server->rejected_conn) + "</h3>";
        resp_data += "<h3>Current conn num:" + std::to_string(m_rpc_server->m_conn_map.size()) + "</h3>";
        for(auto conn: m_rpc_server->m_conn_set) {
          resp_data += "&nbsp;&nbsp;conn id:" + conn.first + " fd:" + std::to_string(conn.second->getFd()) + "</br>";
        }
        resp_data += "<h3>Seqid:" + std::to_string(m_rpc_server->m_seqid) + "</h3>";
        resp_data += "<h3>Req Q size:" + std::to_string(m_rpc_server->m_request_q.size()) + "</h3>";
        resp_data += "<h3>Req InQueue fail num:" + std::to_string(m_rpc_server->req_inqueue_fail) + "</h3>";
        resp_data += "<h3>Resp Q size:" + std::to_string(m_rpc_server->m_response_q.size()) + "</h3>";
        resp_data += "<h3>Resp InQueue fail num:" + std::to_string(m_rpc_server->resp_inqueue_fail) + "</h3>";
        resp_data += "<h3>Total Req num:" + std::to_string(m_rpc_server->total_req_num) + "</h3>";
        resp_data += "<h3>Total Resp num:" + std::to_string(m_rpc_server->total_resp_num) + "</h3>";
        resp_data += "<h3>Total Call num:" + std::to_string(m_rpc_server->total_call_num) + "</h3>";
        resp_data += "<h3>Avg Req wait time:" + std::to_string(m_rpc_server->avg_req_wait_time) + "ms</h3>";
        resp_data += "<h3>Avg Resp wait time:" + std::to_string(m_rpc_server->avg_resp_wait_time) + "ms</h3>";
        resp_data += "<h3>Avg Call time:" + std::to_string(m_rpc_server->avg_call_time) + "ms</h3>";
        resp_data += "<h3>Longest Call time:" + std::to_string(m_rpc_server->max_call_time) + "ms</h3>";
        resp_data += "<h3>epoll fd:" + std::to_string(m_rpc_server->m_epoll_fd) + "</h3>";
        resp_data += "<h3>Rpc listening on port:" + std::to_string(m_rpc_server->m_cfg.m_port) + " fd:" + std::to_string(m_rpc_server->m_listen_socket) + "</h3>";
        resp_data += "<h1>Service Status</h1>";
        for(auto srv: m_rpc_server->m_service_map) {
          resp_data += "<h2>service name:" + srv.first + "<h2>";
          std::vector<std::string> mnames;
          for (auto &method: srv.second->m_method_map) {
            resp_data += "<h3>&nbsp;&nbsp;method name:" + method.first + "</h3>";
            if(method.second.m_status->enabled) {
              resp_data += "&nbsp;&nbsp;&nbsp;&nbsp;Total Call num:" + std::to_string(method.second.m_status->total_call_nums) + "</br>";
              resp_data += "&nbsp;&nbsp;&nbsp;&nbsp;Timeout Call num:" + std::to_string(method.second.m_status->timeout_call_nums) + "</br>";
              resp_data += "&nbsp;&nbsp;&nbsp;&nbsp;Avg Call time:" + std::to_string(method.second.m_status->avg_call_time) + "ms</br>";
              resp_data += "&nbsp;&nbsp;&nbsp;&nbsp;Longest Call time:" + std::to_string(method.second.m_status->longest_call_time) + "ms</br>";
              resp_data += "&nbsp;&nbsp;&nbsp;&nbsp;Call from http num:" + std::to_string(method.second.m_status->call_from_http_num) + "</br>";
            }
            else {
              resp_data += "&nbsp;&nbsp;&nbsp;&nbsp;Method statistic disabled</br>";
            }
          }
        }
        resp_data += "</br>";
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
