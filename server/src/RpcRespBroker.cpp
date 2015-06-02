/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>

#include "RpcRespBroker.h"
#include "RpcServer.h"
#include "rpc.pb.h"

namespace rpcframe
{

RpcRespBroker::RpcRespBroker(RpcServer *server, const std::string conn_id, const std::string req_id, bool needResp) 
: m_server(server)
, m_conn_id(conn_id)
, m_req_id(req_id)
, m_need_resp(needResp)
{
};

bool RpcRespBroker::response(const std::string &resp_data) {
    if(m_need_resp) {
        RpcInnerResp resp;
        resp.set_request_id(m_req_id);
        resp.set_data(resp_data);
        std::string *out_data = new std::string();
        resp.SerializeToString(out_data);

        rpcframe::response_pkg *resp_pkg = new rpcframe::response_pkg();
        resp_pkg->data = out_data;
        resp_pkg->data_len = out_data->length();

        //put response to connection queue
        m_server->pushResp(m_conn_id, resp_pkg);
    }
    else {
        return false;
    }
    return true;
}
};
