/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include "RpcRespBroker.h"

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>
#include <arpa/inet.h>

#include "RpcPackage.h"
#include "RpcServerConnWorker.h"
#include "util.h"

namespace rpcframe
{

RpcRespBroker::RpcRespBroker(RpcServerConnWorker *conn_worker,
                             const std::string &conn_id, 
                             const std::string &req_id, 
                             bool needResp,
                             bool is_from_http) 
: m_connworker(conn_worker)
, m_conn_id(conn_id)
, m_req_id(req_id)
, m_need_resp(needResp)
, m_is_responed(false)
, m_from_http(is_from_http)
, m_return_val_pos(0)
{
  m_resp_proto.set_version(RPCFRAME_VER);
  m_resp_proto.set_request_id(m_req_id);
};

bool RpcRespBroker::isNeedResp() {
    return m_need_resp;
}

bool RpcRespBroker::isResponed() {
  return m_is_responed;
}

bool RpcRespBroker::isAlloced() {
  return m_return_val_pos != 0;
}

bool RpcRespBroker::isFromHttp() {
    return m_from_http;
}

bool RpcRespBroker::response(RpcStatus rs) {
    if(!m_need_resp) {
        return false;
    }
    if (m_from_http) {
        allocHttpResp(getHttpCode(rs), m_resp_pkg->data);
    }
    else {
        //put response to connection queue
        setReturnVal(rs);
    }
    m_is_responed = true;
    m_connworker->pushResp(m_conn_id, *this);
    return true;
}

char *RpcRespBroker::allocRespBufFrom(const std::string &resp)
{
    if (m_from_http) {
        return allocHttpResp(200, resp);
    }
    else {
        char *p_ss = allocRespBuf(resp.size() + 1);
        strcpy(p_ss, resp.c_str());
        p_ss[resp.size()] = '\0';
        return p_ss;
    }
}

char *RpcRespBroker::allocRespBufFrom(const google::protobuf::Message &resp)
{
    char *p_ss = nullptr;
    if (m_from_http) {
        p_ss = allocHttpBuf(200, resp.ByteSize());
        if(!resp.SerializeToArray(p_ss, resp.ByteSize())) {
            return nullptr;
        }
    }
    else {
        p_ss = allocRespBuf(resp.ByteSize());
        if(!resp.SerializeToArray(p_ss, resp.ByteSize())) {
            return nullptr;
        }
    }
    return p_ss;
}

char *RpcRespBroker::allocRespBuf(size_t len)
{
    if (m_from_http) {
        return allocHttpBuf(200, len);
    }
    else {
        uint32_t proto_size = m_resp_proto.ByteSize();
        //proto len header + protobuf data + user data + return value
        size_t pkg_size = sizeof(uint32_t) + proto_size + len + sizeof(uint32_t);
        //TODO:always realloc is slow
        m_resp_pkg.reset(new response_pkg(pkg_size + sizeof(uint32_t)));
        char *writer = m_resp_pkg->data;
        //write total len
        uint32_t pkg_hdr = htonl(pkg_size);
        uint32_t proto_hdr = htonl(proto_size);
        memcpy((void *)writer, (const void *)(&pkg_hdr), sizeof(pkg_hdr));
        memcpy((void *)(writer + sizeof(pkg_hdr)), (const void *)(&proto_hdr), sizeof(proto_hdr));
        if (!m_resp_proto.SerializeToArray((void *)(writer + sizeof(pkg_hdr) + sizeof(proto_hdr)), proto_size)) {
            RPC_LOG(RPC_LOG_LEV::ERROR, "Serialize req data fail");
            return nullptr;
        }
        m_user_data_pos = sizeof(pkg_hdr) + sizeof(proto_hdr) + proto_size;
        m_return_val_pos = m_user_data_pos + len;
        RPC_LOG(RPC_LOG_LEV::DEBUG, "full output buf len %d, data len %d", pkg_size + sizeof(uint32_t), pkg_size);
        return writer + sizeof(pkg_hdr) + sizeof(proto_hdr) + proto_size;
    }
}

void RpcRespBroker::setReturnVal(RpcStatus rs)
{
    if(m_resp_pkg.get() == nullptr && m_return_val_pos == 0) {
        if (!m_from_http) {
            allocRespBuf(1);
        }
        else {
            allocHttpBuf(getHttpCode(rs), 1);
        }
        RPC_LOG(RPC_LOG_LEV::ERROR, "m_resp_pkg not inited, should call allocRespBuf first");
    }
    if (!m_from_http) {
        uint32_t rt_val = htonl(static_cast<uint32_t>(rs));
        memcpy((void *)(m_resp_pkg->data + m_return_val_pos), (const void *)(&rt_val), sizeof(rt_val));
    }
}

RespPkgPtr RpcRespBroker::getRespPkg()
{
  return m_resp_pkg;
}

char *RpcRespBroker::getUserData()
{
  return m_resp_pkg->data + m_user_data_pos;
}

char *RpcRespBroker::allocHttpResp(int status, const std::string &resp) {
    std::stringstream ss;
    setupHttpHdr(ss, status, resp.size());
    ss << resp;
    m_resp_pkg.reset(new response_pkg(ss.str().size()));
    memcpy(m_resp_pkg->data, ss.str().data(), ss.str().size());
    m_return_val_pos = m_resp_pkg->data_len;
    return m_resp_pkg->data;
}

char *RpcRespBroker::allocHttpBuf(int status, size_t len) {
    std::stringstream ss;
    setupHttpHdr(ss, status, len);
    m_resp_pkg.reset(new response_pkg(ss.str().size() + len));
    memcpy(m_resp_pkg->data, ss.str().data(), ss.str().size());
    m_return_val_pos = m_resp_pkg->data_len;
    return m_resp_pkg->data + ss.str().size();
}

int RpcRespBroker::getHttpCode(RpcStatus rs)
{
    switch(rs) {
        case RpcStatus::RPC_SRV_NOTFOUND:
        case RpcStatus::RPC_METHOD_NOTFOUND:
            return 404;
        default:
            return 200;
    }
}

void RpcRespBroker::setupHttpHdr(std::stringstream &ss, int status, size_t len)
{
    ss << "HTTP/1.1 " << status << "\r\n";
    ss << "Content-Type: text/html\r\n";
    ss << "Content-Length: " << len << "\r\n";
    ss << "Connection: close\r\n";
    ss << "\r\n";
}

};
