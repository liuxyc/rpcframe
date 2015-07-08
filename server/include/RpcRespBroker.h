/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCRESPBROKER
#define RPCFRAME_RPCRESPBROKER
#include <string>

#include "IRpcRespBroker.h"

namespace rpcframe {

class RpcServerImpl;

/**
 * @brief RpcRespBroker help server send the response in async
 */
class RpcRespBroker: public IRpcRespBroker
{
public:
    RpcRespBroker(RpcServerImpl *server, const std::string &conn_id, const std::string &req_id, bool needResp);
    bool response(const std::string &resp_data);
    bool isNeedResp();

    RpcRespBroker(const RpcRespBroker &) = delete;
    RpcRespBroker &operator=(const RpcRespBroker &) = delete;
private:
    RpcServerImpl *m_server;
    std::string m_conn_id;
    std::string m_req_id;
    bool m_need_resp;
};

};
#endif
