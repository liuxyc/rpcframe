/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCRESPBROKER
#define RPCFRAME_RPCRESPBROKER
#include <string>

namespace rpcframe {

class RpcServer;

class RpcRespBroker
{
public:
    RpcRespBroker(RpcServer *server, const std::string conn_id, const std::string req_id, bool needResp);
    bool response(const std::string &resp_data);
private:
    RpcServer *m_server;
    std::string m_conn_id;
    std::string m_req_id;
    bool m_need_resp;
};

};
#endif
