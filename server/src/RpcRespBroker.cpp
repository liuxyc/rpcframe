/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>

#include "RpcRespBroker.h"
#include "RpcPackage.h"
#include "RpcServerImpl.h"
#include "IService.h"
#include "rpc.pb.h"

namespace rpcframe
{

RpcRespBroker::RpcRespBroker(RpcServerImpl *server, 
                             const std::string &conn_id, 
                             const std::string &req_id, 
                             bool needResp,
                             mg_connection *http_conn) 
: m_server(server)
, m_conn_id(conn_id)
, m_req_id(req_id)
, m_need_resp(needResp)
, m_http_conn(http_conn)
{
};

bool RpcRespBroker::isNeedResp() {
    return m_need_resp;
}

bool RpcRespBroker::isFromHttp() {
    return (m_conn_id == "http_connection");
}

bool RpcRespBroker::response(const std::string &resp_data) {
    if(m_need_resp) {
        if (m_http_conn != NULL) {
            sendHttpResp(m_http_conn, 200, resp_data);
        }
        else {
            RpcInnerResp resp;
            resp.set_request_id(m_req_id);
            resp.set_ret_val(static_cast<uint32_t>(RpcStatus::RPC_SERVER_OK));
            resp.set_data(resp_data);
            response_pkg *resp_pkg = new response_pkg(resp.ByteSize());
            resp.SerializeToArray(resp_pkg->data, resp_pkg->data_len);

            //put response to connection queue
            m_server->pushResp(m_conn_id, resp_pkg);
        }
    }
    else {
        return false;
    }
    return true;
}

void RpcRespBroker::sendHttpResp(mg_connection *conn, int status, const std::string &resp) {
    mg_send_status(conn, status);
    mg_send_header(conn, "Content-Type", "text/plain");
    mg_send_header(conn, "Content-Length", std::to_string(resp.size()).c_str());
    mg_send_header(conn, "Connection", "close");
    mg_write(conn, "\r\n", 2);
    mg_printf(conn, resp.c_str());
}

};
