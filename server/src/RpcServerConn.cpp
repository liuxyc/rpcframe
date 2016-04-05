/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
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

RpcServerConn::RpcServerConn(int fd, uint32_t seqid, RpcServerImpl *server)
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
    m_seqid += std::to_string(seqid) + "_";
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

bool RpcServerConn::readPkgLen(uint32_t &pkg_len)
{
    if (!is_connected) {
        RPC_LOG(RPC_LOG_LEV::WARNING, "connection already disconnected");
        return false;
    }
    int data = 0;
    errno = 0;
    int left_size = sizeof(data);
    char *data_ptr = (char *)&data;
    while(1) {
      int rev_size = recv(m_fd, data_ptr + (sizeof(data) - left_size), left_size, 0);  
      if (rev_size <= 0) {
        if (rev_size != 0) {
          RPC_LOG(RPC_LOG_LEV::ERROR, "try recv pkg len error %s", strerror(errno));
        }
        if( errno == EAGAIN || errno == EINTR) {
          //ET trigger case
          //will continue until we read the full package length
          RPC_LOG(RPC_LOG_LEV::DEBUG, "recv data too small %d, try again", rev_size);
        }
        else {
          return false;
        }
      }
      left_size -= rev_size;
      if(left_size == 0) {
        pkg_len = ntohl(data);
        return true;
      }
      if( left_size > 0 )
      {  
        RPC_LOG(RPC_LOG_LEV::DEBUG, "recv data too small %d, try againi %d left", rev_size, left_size);
        continue;
      }  
      if( left_size < 0 )
      {  
        RPC_LOG(RPC_LOG_LEV::DEBUG, "recv wrong hdr %d, close fd %d", rev_size, m_fd);
        return false;
      }  
    }
    return false;
}

PkgIOStatus RpcServerConn::readPkgData()
{
    if (m_rpk == nullptr) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "rpk is nullptr");
        return PkgIOStatus::FAIL;
    }
    if (!is_connected) {
        RPC_LOG(RPC_LOG_LEV::WARNING, "connection already disconnected");
        return PkgIOStatus::FAIL;
    }
    errno = 0;
    while(1) {
      int rev_size = recv(m_fd, m_rpk->data + (m_cur_pkg_size - m_cur_left_len), m_cur_left_len, 0);  
      if (rev_size <= 0) {
        if( errno == EAGAIN) {
          //for ET case
          m_cur_left_len -= rev_size;
          return PkgIOStatus::PARTIAL;
        }
        else if(  errno == EINTR ) {
        }
        else {
          RPC_LOG(RPC_LOG_LEV::ERROR, "recv pkg error %s", strerror(errno));
          return PkgIOStatus::FAIL;
        }
      }
      if ((uint32_t)rev_size == m_cur_left_len) {
        //RPC_LOG(RPC_LOG_LEV::DEBUG, "got full pkg");
        m_cur_left_len = 0;
        m_cur_pkg_size = 0;
        return PkgIOStatus::FULL;
      }
      else {
        m_cur_left_len -= rev_size;
        //RPC_LOG(RPC_LOG_LEV::DEBUG, " half pkg got %d/%d need %d more", rev_size, m_cur_pkg_size, m_cur_left_len);
        //NOTICE:consider as EAGAIN
        return PkgIOStatus::PARTIAL;
      }
    }
}

pkg_ret_t RpcServerConn::getRequest()
{
    if (m_cur_pkg_size == 0) {
        //new pkg
        if (!readPkgLen(m_cur_pkg_size)) {
            return pkg_ret_t(-1, nullptr);
        }
        if (m_cur_pkg_size == 0) {
            return pkg_ret_t(0, nullptr);
        }
        //RPC_LOG(RPC_LOG_LEV::DEBUG, "pkg len is %d", m_cur_pkg_size);
        if(m_cur_pkg_size > MAX_REQ_LIMIT_BYTE) {
          RPC_LOG(RPC_LOG_LEV::WARNING, "request %s too large %llu > %llu", m_seqid.c_str(), m_cur_pkg_size, MAX_REQ_LIMIT_BYTE);
          return pkg_ret_t(-1, nullptr);
        }
        //TODO: may produce many memory fragment here
        m_rpk = new request_pkg(m_cur_pkg_size, m_seqid);
        if (m_rpk == nullptr) {
          RPC_LOG(RPC_LOG_LEV::ERROR, "malloc request fail %s, size %llu", m_seqid.c_str(), m_cur_pkg_size);
          return pkg_ret_t(-1, nullptr);
        }
        m_cur_left_len = m_cur_pkg_size;
    }
    PkgIOStatus data_ret = readPkgData();
    if (data_ret == PkgIOStatus::FAIL) {
        return pkg_ret_t(-1, nullptr);
    }
    else if( data_ret == PkgIOStatus::PARTIAL) {
        return pkg_ret_t(0, nullptr);
    }
    else {
        std::shared_ptr<request_pkg> p_rpk(m_rpk);
        //FIXME:
        m_rpk = nullptr;
        return pkg_ret_t(0, p_rpk);
    }
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
      if (errno == EAGAIN) {
        //ET trigger case
        m_sent_len += slen;
        return PkgIOStatus::PARTIAL;
      }
      if (slen == 0 || errno == EPIPE) {
        RPC_LOG(RPC_LOG_LEV::WARNING, "peer closed");
        return PkgIOStatus::FAIL;
      }
      if( errno == EINTR) {
        m_sent_len += slen;
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
      //RPC_LOG(RPC_LOG_LEV::DEBUG, "full send %d", m_sent_len);
      m_sent_pkg = nullptr;
      m_sent_len = 0;
      return PkgIOStatus::FULL;
    }
    else {
      //RPC_LOG(RPC_LOG_LEV::DEBUG, "left send %d", m_sent_pkg->data_len - m_sent_len);
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
    else {
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
            return sendData();
        }
    }
    return PkgIOStatus::PARTIAL;
}

bool RpcServerConn::isSending() const {
    return (m_sent_pkg != nullptr);
}

};
