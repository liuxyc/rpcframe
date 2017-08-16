/*
 * Copyright (c) 2015-2017, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include "RpcServerRpcConn.h"

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

RpcServerRpcConn::RpcServerRpcConn(int fd, const char *id, RpcServerImpl *server)
: RpcServerConn(fd, id, server)
{
}

RpcServerRpcConn::~RpcServerRpcConn()
{
}


bool RpcServerRpcConn::readPkgLen(uint32_t &pkg_len)
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
      if (rev_size == 0 && left_size != 0) {
        RPC_LOG(RPC_LOG_LEV::INFO, "Peer disconected");
        return false;
      }
      if (rev_size < 0) {
        if( errno == EAGAIN || errno == EINTR) {
          //ET trigger case
          //will continue until we read the full package length
          //RPC_LOG(RPC_LOG_LEV::DEBUG, "recv data too small %d, try again", rev_size);
          if( left_size == sizeof(data) ) {
            //no avaliable data
            return true;
          }
        }
        else {
          RPC_LOG(RPC_LOG_LEV::ERROR, "try recv pkg len error %s, need recv %d:%d", strerror(errno), left_size, rev_size);
          return false;
        }
      }
      else {
        left_size -= rev_size;
        if(left_size == 0) {
          pkg_len = ntohl(data);
          return true;
        }
        if( left_size > 0 )
        {  
          RPC_LOG(RPC_LOG_LEV::DEBUG, "recv data too small %d, try again %d left", rev_size, left_size);
          continue;
        }  
        if( left_size < 0 )
        {  
          RPC_LOG(RPC_LOG_LEV::DEBUG, "recv wrong hdr %d, close fd %d", rev_size, m_fd);
          return false;
        }  
      }
    }
    return false;
}

PkgIOStatus RpcServerRpcConn::readPkgData()
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
        if(rev_size == 0 && m_cur_left_len !=0) {
          RPC_LOG(RPC_LOG_LEV::ERROR, "peer disconnected");
          return PkgIOStatus::FAIL;
        }
        if( errno == EAGAIN) {
          //for ET case
          RPC_LOG(RPC_LOG_LEV::DEBUG, " half pkg got %d/%d need %d more", rev_size, m_cur_pkg_size, m_cur_left_len);
          return PkgIOStatus::PARTIAL;
        }
        else if(errno == EINTR) {
          continue;
        }
        else {
          RPC_LOG(RPC_LOG_LEV::ERROR, "recv pkg error %s", strerror(errno));
          return PkgIOStatus::FAIL;
        }
      }
      if ((uint32_t)rev_size == m_cur_left_len) {
        RPC_LOG(RPC_LOG_LEV::DEBUG, "got full pkg");
        m_cur_left_len = 0;
        m_cur_pkg_size = 0;
        return PkgIOStatus::FULL;
      }
      else {
        m_cur_left_len -= rev_size;
        RPC_LOG(RPC_LOG_LEV::DEBUG, " half pkg got %d/%d need %d more", rev_size, m_cur_pkg_size, m_cur_left_len);
        //NOTICE:consider as EAGAIN
        return PkgIOStatus::PARTIAL;
      }
    }
}

pkg_ret_t RpcServerRpcConn::getRequest()
{
    if (m_cur_pkg_size == 0) {
        //new pkg
        if (!readPkgLen(m_cur_pkg_size)) {
            return pkg_ret_t(-1, nullptr);
        }
        if (m_cur_pkg_size == 0) {
            return pkg_ret_t(0, nullptr);
        }
        RPC_LOG(RPC_LOG_LEV::DEBUG, "pkg len is %d", m_cur_pkg_size);
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
        m_rpk = nullptr;
        return pkg_ret_t(0, p_rpk);
    }
}

};
