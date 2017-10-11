/*
 * Copyright (c) 2015-2017, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include "RpcClientConn.h"
#include "RpcEventLooper.h"

#include <errno.h>
#include <fcntl.h>  
#include <netinet/in.h>  
#include <netinet/tcp.h>  
#include <arpa/inet.h>  


#include "rpc.pb.h"
#include "util.h"

namespace rpcframe
{

RpcClientConn::RpcClientConn(const Endpoint &ep, int connect_timeout, RpcEventLooper *evlooper)
: m_fd(-1)
, m_cur_left_len(0)
, m_cur_pkg_size(0)
, m_rpk(nullptr)
, is_connected(true)
, m_ep(ep)
, m_connect_timeout(connect_timeout)
, m_evlooper(evlooper)
{
}

RpcClientConn::~RpcClientConn()
{
    RPC_LOG(RPC_LOG_LEV::DEBUG, "~RpcClientConn() dest:%s:%d fd %d", m_ep.first.c_str(), m_ep.second, m_fd);
    setInvalid();
}


int RpcClientConn::getFd() const
{
    return m_fd;
}

void RpcClientConn::setInvalid()
{
    ::close(m_fd);
    if (m_rpk != nullptr)
        delete m_rpk;
    m_fd = -1;
    m_last_invalid_time = std::time(nullptr);
}

bool RpcClientConn::isValid()
{
    return m_fd > 0;
}

bool RpcClientConn::shouldRetry()
{
    if(m_fd == -1) {
        return (std::time(nullptr) - m_last_invalid_time) > 60;
    }
    else {
        return false;
    }
}

bool RpcClientConn::connect() 
{
    if(m_fd != -1)
        return true;

    if (connectHost(m_ep.first.c_str(), m_ep.second, m_connect_timeout) == -1)
    {
        m_fd = -1;
        m_last_invalid_time = std::time(nullptr);
        return false;
    }

    int keepAlive = 1;   
    int keepIdle = 60;   
    int keepInterval = 5;   
    int keepCount = 3;   
    setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepAlive, sizeof(keepAlive));  
    setsockopt(m_fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));  
    setsockopt(m_fd, SOL_TCP, TCP_KEEPINTVL, (void*)&keepInterval, sizeof(keepInterval));  
    setsockopt(m_fd, SOL_TCP, TCP_KEEPCNT, (void*)&keepCount, sizeof(keepCount));  
    int flag = 1;
    if(setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag) ) == -1){
        RPC_LOG(RPC_LOG_LEV::WARNING, "couldn't setsockopt(tcp_nodelay)");
    }
    m_evlooper->addConnection(m_fd, this);
    RPC_LOG(RPC_LOG_LEV::INFO, "RpcClientConn connected: %s:%d %d", m_ep.first.c_str(), m_ep.second, m_fd);
    return true;
}

int RpcClientConn::setNoBlocking(int fd) 
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option|O_NONBLOCK;
    fcntl(fd,F_SETFL, new_option);
    return old_option;
}


int RpcClientConn::connectHost(const char* hostname, int port, int timeoutv) 
{
    int ret = 0;
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    /* Obtain address(es) matching host/port */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;          /* Any protocol */

    ret = getaddrinfo(hostname, std::to_string(port).c_str(), &hints, &result);
    if (ret != 0) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "getaddrinfo:%s", gai_strerror(ret));
        return -1;
    }

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully connect(2).
       If socket(2) (or connect(2)) fails, we (close the socket
       and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        m_fd = socket(rp->ai_family, rp->ai_socktype,
                rp->ai_protocol);
        if (m_fd == -1)
            continue;

        if (noBlockConnect(m_fd, rp->ai_addr, rp->ai_addrlen, timeoutv) != -1)
            break;                  /* Success */

        close(m_fd);
        m_fd = -1;
    }

    if (rp == NULL) {               /* No address succeeded */
        RPC_LOG(RPC_LOG_LEV::ERROR, "can not connect");
        return -1;
    }

    freeaddrinfo(result);           /* No longer needed */    
    return 0;
}


int RpcClientConn::noBlockConnect(int sockfd, struct sockaddr *address, socklen_t addr_len, int timeoutv) 
{
    int ret = 0;
    int fdopt = setNoBlocking(sockfd);
    ret = ::connect(sockfd, address, addr_len);
    if(ret == 0) {
        fcntl(sockfd, F_SETFL,fdopt);
        return sockfd;
    }
    else if(errno != EINPROGRESS) {//if errno not EINPROGRESS, errror
        RPC_LOG(RPC_LOG_LEV::ERROR, "unblock connect not support");
        ::close(sockfd);
        return -1;
    }
    fd_set writefds;
    struct timeval timeout;//connect time out
    FD_ZERO(&writefds);
    FD_SET(sockfd, &writefds);
    timeout.tv_sec = timeoutv;
    timeout.tv_usec = 0;
    ret = ::select(sockfd+1, nullptr, &writefds, nullptr, &timeout);
    if(ret <= 0) {
      if(ret == 0) { 
        RPC_LOG(RPC_LOG_LEV::ERROR, "connect %s:%d time out", m_ep.first.c_str(), m_ep.second);
        close(sockfd);
        return -1;
      }
      else {
        RPC_LOG(RPC_LOG_LEV::ERROR, "select connect server %s:%d error:%s", m_ep.first.c_str(), m_ep.second, strerror(errno));
      }
    }
    if(!FD_ISSET(sockfd, &writefds)) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "no events on sockfd found");
        close(sockfd);
        return -1;
    }
    int error = 0;
    socklen_t length = sizeof(error);
    if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &length) < 0) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "get socket option error");
        close(sockfd);
        return -1;
    }
    if(error != 0 ) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "connect server %s:%d error:%s", m_ep.first.c_str(), m_ep.second, strerror(error));
        close(sockfd);
        return -1;
    }
    //set socket back to block
    fcntl(sockfd, F_SETFL, fdopt); 
    return 0;
}


bool RpcClientConn::readPkgLen(uint32_t &pkg_len)
{
    if (!is_connected) {
        RPC_LOG(RPC_LOG_LEV::WARNING, "connection already disconnected");
        return false;
    }
    uint32_t data;
    char *p = (char *)(&data);
    int recv_left = sizeof(data);
    int recved = 0;
    while(true) {
        int rev_size = recv(m_fd, p + recved, recv_left, 0);  
        if (rev_size <= 0) {
            if (rev_size == 0) {
                RPC_LOG(RPC_LOG_LEV::WARNING, "recv pkg peer close");
                return false;
            }
            if( errno == EAGAIN || errno == EINTR) {
                RPC_LOG(RPC_LOG_LEV::INFO, "recv pkg interupt %s", strerror(errno));
            }
            else {
                RPC_LOG(RPC_LOG_LEV::ERROR, "recv pkg error %s", strerror(errno));
                return false;
            }
        }
        recved += rev_size;
        if( recved == sizeof(data) )  
        {  
            RPC_LOG(RPC_LOG_LEV::DEBUG, "recv full len");
            break;
        }
        recv_left -= rev_size;
        RPC_LOG(RPC_LOG_LEV::INFO, "need more len %d", recv_left);
    }
    pkg_len = ntohl(data);
    RPC_LOG(RPC_LOG_LEV::DEBUG, "got resp len %lu", pkg_len);
    return true;
}

PkgIOStatus RpcClientConn::readPkgData()
{
    if (m_rpk == nullptr) {
        RPC_LOG(RPC_LOG_LEV::ERROR, "rpk is nullptr");
        return PkgIOStatus::FAIL;
    }
    if (!is_connected) {
        RPC_LOG(RPC_LOG_LEV::WARNING, "connection already disconnected");
        return PkgIOStatus::FAIL;
    }
    int rev_size = recv(m_fd, m_rpk->data + (m_cur_pkg_size - m_cur_left_len), m_cur_left_len, MSG_DONTWAIT);  
    if (rev_size <= 0) {
        if (rev_size == 0) {
            RPC_LOG(RPC_LOG_LEV::WARNING, "recv pkg peer close");
            return PkgIOStatus::FAIL;
        }
        if( errno == EAGAIN || errno == EINTR) {
            RPC_LOG(RPC_LOG_LEV::INFO, "recv pkg interupt %s", strerror(errno));
            return PkgIOStatus::PARTIAL;
        }
        else {
            RPC_LOG(RPC_LOG_LEV::ERROR, "recv pkg error %s", strerror(errno));
            return PkgIOStatus::FAIL;
        }
    }
    if ((uint32_t)rev_size == m_cur_left_len) {
        RPC_LOG(RPC_LOG_LEV::DEBUG, "got full pkg %lu", rev_size);
        m_cur_left_len = 0;
        m_cur_pkg_size = 0;
        return PkgIOStatus::FULL;
    }
    else {
        m_cur_left_len -= rev_size;
        RPC_LOG(RPC_LOG_LEV::DEBUG, " half pkg got %d need %d more", rev_size, m_cur_left_len);
        return PkgIOStatus::PARTIAL;
    }
}

pkg_ret_t RpcClientConn::getResponse()
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
        m_rpk = new response_pkg(m_cur_pkg_size);
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
        RespPkgPtr resp_pkg(m_rpk);
        m_rpk = nullptr;
        return pkg_ret_t(0, resp_pkg);
    }
}

RpcStatus RpcClientConn::sendReq( const std::string &service_name, const std::string &method_name, 
        const RawData &request_data, const std::string &reqid, bool is_oneway, uint32_t timeout) 
{

  RpcInnerReq req;
  req.set_version(RPCFRAME_VER);
  req.set_service_name(service_name);
  req.set_method_name(method_name);
  req.set_request_id(reqid);
  req.set_timeout(timeout);
  bool hasBlob = true;
  if(request_data.size() > 0 && request_data.size() < max_protobuf_data_len) {
    req.set_data(request_data.data, request_data.data_len);
    hasBlob = false;
  }
  req.set_type(is_oneway?RpcInnerReq::ONE_WAY:RpcInnerReq::TWO_WAY);
  int proto_len = req.ByteSize();
  size_t pkg_len = 0;
  pkg_len = sizeof(proto_len) + proto_len;
  if(hasBlob){
    pkg_len += request_data.size();
  }
  uint32_t pkg_hdr = htonl(pkg_len);
  uint32_t proto_hdr = htonl(proto_len);
  std::unique_ptr<char[]> out_data(new char[pkg_len + sizeof(pkg_hdr) + sizeof(proto_hdr)]);
  memcpy((void *)(out_data.get()), (const void *)(&pkg_hdr), sizeof(pkg_hdr));
  memcpy((void *)(out_data.get() + sizeof(pkg_hdr)), (const void *)(&proto_hdr), sizeof(proto_hdr));
  if (!req.SerializeToArray((void *)(out_data.get() + sizeof(pkg_hdr) + sizeof(proto_hdr)), proto_len)) {
    RPC_LOG(RPC_LOG_LEV::ERROR, "Serialize req data fail");
    return RpcStatus::RPC_SEND_FAIL;
  }
  if(hasBlob){
    memcpy((void *)(out_data.get() + sizeof(pkg_hdr)+ sizeof(proto_hdr) + proto_len), request_data.data, request_data.size());
  }

  RpcStatus rc = sendData(out_data.get(), pkg_len + sizeof(pkg_hdr), timeout);
  return rc;
}

RpcStatus RpcClientConn::sendData(char *data, size_t len, uint32_t timeout)
{
  std::time_t begin_tm = std::time(nullptr);
  uint32_t total_len = len;
  uint32_t sent_len = 0;
  while(true) {
    if (timeout > 0 &&
        std::time(nullptr) - begin_tm > timeout) {
      RPC_LOG(RPC_LOG_LEV::WARNING, "send data timeout!");
      return RpcStatus::RPC_SEND_TIMEOUT;
    }
    uint32_t len = total_len - sent_len;
    errno = 0;
    int s_ret = send(m_fd, data + sent_len, len, MSG_NOSIGNAL | MSG_DONTWAIT);
    if( s_ret <= 0)
    {
      if( errno == EAGAIN || errno == EINTR) {
        continue;
      }
      else {
        RPC_LOG(RPC_LOG_LEV::ERROR, "send data error! %s, %u, %u, %d %u", 
            strerror(errno), total_len, sent_len, s_ret, len);
        is_connected = false;
        return RpcStatus::RPC_SEND_FAIL;
      }
    }
    sent_len += s_ret;
    if (sent_len == total_len) {
      break;
    }

  }
  return RpcStatus::RPC_SEND_OK;
}

};
