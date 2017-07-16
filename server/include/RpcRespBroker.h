/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include <string>

#include "IRpcRespBroker.h"
#include "rpc.pb.h"
#include "RpcPackage.h"
#include "RpcDefs.h"
class mg_connection;

namespace rpcframe {

class RpcServerConnWorker;
class RpcInnerResp;

/**
 * @brief RpcRespBroker help server send the response in async
 */
class RpcRespBroker: public IRpcRespBroker
{
public:
    RpcRespBroker(RpcServerConnWorker *conn_worker, const std::string &conn_id, const std::string &req_id, bool needResp, bool is_from_http);
    bool response(RpcStatus rs) override;
    bool isNeedResp() override;
    bool isResponed() override;
    bool isFromHttp() override;
    char *allocRespBuf(size_t len) override;
    char *allocRespBufFrom(const std::string &resp) override;
    void setReturnVal(RpcStatus rs) override;

    RespPkgPtr getRespPkg();
    char *getUserData();
    bool isAlloced();
    int getHttpCode(RpcStatus rs);
    void setupHttpHdr(std::stringstream &ss, int status, size_t len);

    RpcRespBroker(const RpcRespBroker &) = delete;
    RpcRespBroker &operator=(const RpcRespBroker &) = delete;
private:
    char *allocHttpResp(int status, const std::string &resp);
    char *allocHttpBuf(int status, size_t len);
    RpcServerConnWorker *m_connworker;
    std::string m_conn_id;
    std::string m_req_id;
    bool m_need_resp;
    bool m_is_responed;
    bool m_from_http;
    RpcInnerResp m_resp_proto;
    RespPkgPtr m_resp_pkg;
    uint32_t m_return_val_pos;
    uint32_t m_user_data_pos;
};

};
