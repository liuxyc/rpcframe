/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include "RpcServerConn.h"

#include <errno.h>
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <fcntl.h>  
#include <string.h>
#include <unistd.h>

#include <chrono>

#include "util.h"
#include "RpcServerImpl.h"
#include "RpcServerConfig.h"

namespace rpcframe
{

RpcServerConn::RpcServerConn(int fd, const char *id, RpcServerImpl *server)
: m_fd(fd)
, m_cur_left_len(0)
, m_cur_pkg_size(0)
, m_rpk(nullptr)
, is_connected(true)
, m_sent_len(0)
, m_sent_pkg(nullptr)
, MAX_REQ_LIMIT_BYTE(server->getConfig()->m_max_req_size)
, m_server(server)
{
    //generate a connection id, this id used for track and identify RpcServerConn instance
    m_seqid = std::to_string(std::time(nullptr)) + "_";
    m_seqid += id;
    m_seqid += "_";
    m_seqid += std::to_string(fd);
}

RpcServerConn::~RpcServerConn()
{
    RPC_LOG(RPC_LOG_LEV::INFO, "close fd %d", m_fd);
    close(m_fd);
    if (m_rpk != nullptr)
        delete m_rpk;
}

void RpcServerConn::reset()
{
    std::lock_guard<std::mutex> lck(m_mutex);
    is_connected = false;
    m_cur_left_len = 0;
    m_cur_pkg_size = 0;
    if (m_rpk != nullptr) {
        delete m_rpk;
    }
    m_rpk = nullptr;
}

int RpcServerConn::getFd() const 
{
    return m_fd;
}

PkgIOStatus RpcServerConn::sendData()
{
  while(1) {
    int data_to_send = m_sent_pkg->data_len - m_sent_len;
    int slen = send(m_fd, 
        m_sent_pkg->data + m_sent_len, 
        data_to_send, 
        MSG_NOSIGNAL | MSG_DONTWAIT);  
    //for <= 0 cases
    if (slen <= 0) {
      if (slen == 0 || errno == EPIPE) {
        RPC_LOG(RPC_LOG_LEV::INFO, "peer closed");
        return PkgIOStatus::FAIL;
      }
      if (errno == EAGAIN) {
        //ET trigger case
        //RPC_LOG(RPC_LOG_LEV::INFO, "send data AGAIN");
        return PkgIOStatus::PARTIAL;
      }
      if( errno == EINTR) {
        continue;
      }
      else {
        RPC_LOG(RPC_LOG_LEV::ERROR, "send data error! %s", strerror(errno));
        return PkgIOStatus::FAIL;
      }
    }
    //for > 0 case
    m_sent_len += slen;
    if (m_sent_pkg->data_len == m_sent_len) {
      RPC_LOG(RPC_LOG_LEV::DEBUG, "full send %d", m_sent_len);
      m_sent_pkg = nullptr;
      m_sent_len = 0;
      return PkgIOStatus::FULL;
    }
    else {
      RPC_LOG(RPC_LOG_LEV::DEBUG, "left send %d", m_sent_pkg->data_len - m_sent_len);
      //NOTICE:consider as EAGAIN
      return PkgIOStatus::PARTIAL;
    }
  }
}

PkgIOStatus RpcServerConn::sendResponse()
{
    if (!is_connected) {
        RPC_LOG(RPC_LOG_LEV::WARNING, "send connection already disconnected");
        return PkgIOStatus::FAIL;
    }
    if (m_sent_pkg != nullptr) {
        return sendData();
    }
    RespPkgPtr pkg = nullptr;
    if (m_response_q.pop(pkg, 0)) {
        m_sent_len = 0;
        m_sent_pkg = pkg;
        std::chrono::system_clock::time_point out_q_timepoint = std::chrono::system_clock::now();
        RPC_LOG(RPC_LOG_LEV::DEBUG, "resp stay: %d ms",  std::chrono::duration_cast<std::chrono::milliseconds>( out_q_timepoint - pkg->gen_time ).count());
        auto during = std::chrono::duration_cast<std::chrono::milliseconds>(out_q_timepoint - pkg->gen_time);
        if( during.count() < 0) {
            during = during.zero();
        }
        m_server->calcRespQTime(during.count());
        RPC_LOG(RPC_LOG_LEV::DEBUG, "will send %d", pkg->data_len);
        return sendData();
    }
    return PkgIOStatus::NODATA;
}

bool RpcServerConn::isSending() const {
    return (m_sent_pkg != nullptr);
}

void RpcServerConn::setEpollStruct(EpollStruct *eps)
{
    m_eps = eps;
}

EpollStruct *RpcServerConn::getEpollStruct()
{
    return m_eps;
}

};
