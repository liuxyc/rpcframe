/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCRESPBROKER
#define RPCFRAME_RPCRESPBROKER
#include <string>

#include "IRpcRespBroker.h"
class mg_connection;

namespace rpcframe {

class RpcServerConnWorker;

/**
 * @brief RpcRespBroker help server send the response in async
 */
class RpcRespBroker: public IRpcRespBroker
{
public:
    RpcRespBroker(RpcServerConnWorker *conn_worker, const std::string &conn_id, const std::string &req_id, bool needResp, mg_connection *http_conn);
    bool response(const std::string &resp_data);
    bool isNeedResp();
    bool isFromHttp();

    RpcRespBroker(const RpcRespBroker &) = delete;
    RpcRespBroker &operator=(const RpcRespBroker &) = delete;
private:
    void sendHttpResp(mg_connection *conn, int status, const std::string &resp);
    RpcServerConnWorker *m_connworker;
    std::string m_conn_id;
    std::string m_req_id;
    bool m_need_resp;
    mg_connection *m_http_conn;
};

};
#endif
