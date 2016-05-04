/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include "RpcRespBroker.h"

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>

#include "RpcPackage.h"
#include "mongoose.h"
#include "RpcServerConnWorker.h"
#include "util.h"

namespace rpcframe
{

RpcRespBroker::RpcRespBroker(RpcServerConnWorker *conn_worker,
                             const std::string &conn_id, 
                             const std::string &req_id, 
                             bool needResp,
                             mg_connection *http_conn) 
: m_connworker(conn_worker)
, m_conn_id(conn_id)
, m_req_id(req_id)
, m_need_resp(needResp)
, m_is_responed(false)
, m_http_conn(http_conn)
, m_return_val_pos(0)
{
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
    return (m_conn_id == "http_connection");
}

bool RpcRespBroker::response() {
  if(m_need_resp) {
    if (m_http_conn != nullptr) {
      sendHttpResp(m_http_conn, 200, m_resp_pkg->data);
      m_is_responed = true;
    }
    else {
      //put response to connection queue
      setReturnVal(RpcStatus::RPC_SERVER_OK);
      m_connworker->pushResp(m_conn_id, *this);
      m_is_responed = true;
    }
  }
  else {
    return false;
  }
  return true;
}

char *RpcRespBroker::allocRespBufFrom(const std::string &resp)
{
  char *p_ss = allocRespBuf(resp.size() + 1);
  strcpy(p_ss, resp.c_str());
  p_ss[resp.size()] = '\0';
  return p_ss;
}

char *RpcRespBroker::allocRespBuf(size_t len)
{
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

void RpcRespBroker::setReturnVal(RpcStatus rs)
{
  if(m_resp_pkg.get() == nullptr && m_return_val_pos == 0) {
    allocRespBuf(1);
    RPC_LOG(RPC_LOG_LEV::ERROR, "m_resp_pkg not inited, should call allocRespBuf first");
  }
  uint32_t rt_val = htonl(static_cast<uint32_t>(rs));
  memcpy((void *)(m_resp_pkg->data + m_return_val_pos), (const void *)(&rt_val), sizeof(rt_val));
}

RespPkgPtr RpcRespBroker::getRespPkg()
{
  return m_resp_pkg;
}

char *RpcRespBroker::getUserData()
{
  return m_resp_pkg->data + m_user_data_pos;
}

void RpcRespBroker::sendHttpResp(mg_connection *conn, int status, const std::string &resp) {
    mg_printf(conn, "HTTP/1.1 %d\r\n", status);
    mg_printf(conn, "%s", "Content-Type: text/html\r\n");
    mg_printf(conn, "Content-Length: %lu\r\n", resp.size());
    mg_printf(conn, "%s", "Connection: close\r\n");
    mg_printf(conn, "%s", "\r\n");
    mg_send(conn, resp.c_str(), resp.size());
}

};
