/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include <errno.h>
#include <fcntl.h>  
#include <netinet/in.h>  
#include <string.h>
#include <sys/socket.h>  
#include <unistd.h>
#include <uuid/uuid.h>

#include "RpcClientConn.h"
#include "rpc.pb.h"
#include "util.h"

namespace rpcframe
{

RpcClientConn::RpcClientConn(int fd)
: m_fd(fd)
, m_cur_left_len(0)
, m_cur_pkg_size(0)
, m_rpk(nullptr)
, is_connected(true)
{
}

RpcClientConn::~RpcClientConn()
{
    RPC_LOG(RPC_LOG_LEV::DEBUG, "~RpcClientConn() fd %d", m_fd);
    ::close(m_fd);
    if (m_rpk != nullptr)
        delete m_rpk;
}

int RpcClientConn::getFd() const
{
    return m_fd;
}

bool RpcClientConn::readPkgLen(uint32_t &pkg_len)
{
    if (!is_connected) {
        RPC_LOG(RPC_LOG_LEV::WARNING, "connection already disconnected");
        return false;
    }
    int data;
    char *p = (char *)(&data);
    int recv_left = 4;
    int recved = 0;
    while(true) {
        int rev_size = recv(m_fd, p + recved, recv_left, 0);  
        if (rev_size <= 0) {
            if (rev_size == 0) {
                RPC_LOG(RPC_LOG_LEV::WARNING, "recv pkg peer close %s", strerror(errno));
                return false;
            }
            if( errno == EAGAIN || errno == EINTR) {
                //RPC_LOG(RPC_LOG_LEV::INFO, "recv pkg interupt %s", strerror(errno));
            }
            else {
                RPC_LOG(RPC_LOG_LEV::ERROR, "recv pkg error %s", strerror(errno));
                return false;
            }
        }
        recved += rev_size;
        if( recved == 4 )  
        {  
            //RPC_LOG(RPC_LOG_LEV::DEBUG, "recv full len");
            break;
        }
        recv_left -= rev_size;
        //RPC_LOG(RPC_LOG_LEV::INFO, "need more len %d", recv_left);
    }
    pkg_len = ntohl(data);
    //RPC_LOG(RPC_LOG_LEV::DEBUG, "got resp len %lu", pkg_len);
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
    int rev_size = recv(m_fd, m_rpk->data + (m_cur_pkg_size - m_cur_left_len), m_cur_left_len, 0);  
    if (rev_size <= 0) {
        if (rev_size == 0) {
            RPC_LOG(RPC_LOG_LEV::WARNING, "recv pkg peer close %s", strerror(errno));
            return PkgIOStatus::FAIL;
        }
        if( errno == EAGAIN || errno == EINTR) {
            //RPC_LOG(RPC_LOG_LEV::INFO, "recv pkg interupt %s", strerror(errno));
            return PkgIOStatus::PARTIAL;
        }
        else {
            RPC_LOG(RPC_LOG_LEV::ERROR, "recv pkg error %s", strerror(errno));
            return PkgIOStatus::FAIL;
        }
    }
    if ((uint32_t)rev_size == m_cur_left_len) {
        //RPC_LOG(RPC_LOG_LEV::DEBUG, "got full pkg %lu", rev_size);
        m_cur_left_len = 0;
        m_cur_pkg_size = 0;
        return PkgIOStatus::FULL;
    }
    else {
        m_cur_left_len = m_cur_left_len - rev_size;
        //RPC_LOG(RPC_LOG_LEV::DEBUG, " half pkg got %d need %d more", rev_size, m_cur_left_len);
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
        //RPC_LOG(RPC_LOG_LEV::DEBUG, "pkg len is %d", m_cur_pkg_size);
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
        response_pkg *p_rpk = m_rpk;
        m_rpk = nullptr;
        return pkg_ret_t(0, p_rpk);
    }
}

RpcStatus RpcClientConn::sendReq(
        const std::string &service_name, 
        const std::string &method_name, 
        const std::string &request_data, 
        const std::string &reqid, 
        bool is_oneway, uint32_t timeout) {

    RpcInnerReq req;
    req.set_service_name(service_name);
    req.set_method_name(method_name);

    req.set_request_id(reqid);
    req.set_data(request_data);
    if (is_oneway) {
        req.set_type(RpcInnerReq::ONE_WAY);
    }
    else {
        req.set_type(RpcInnerReq::TWO_WAY);
    }
    std::string out_data;
    req.SerializeToString(&out_data);
    uint32_t pkg_len = out_data.length();
    uint32_t nlen = htonl(pkg_len);

    std::time_t begin_tm = std::time(nullptr);
    uint32_t total_len = sizeof(nlen);
    uint32_t sent_len = 0;
    while(true) {
        if (timeout > 0 && 
            std::time(nullptr) - begin_tm > timeout) {
            RPC_LOG(RPC_LOG_LEV::WARNING, "send data len timeout!");
            return RpcStatus::RPC_SEND_TIMEOUT;
        }
        uint32_t len = total_len - sent_len;
        int s_ret = send(m_fd, ((char *)&nlen) + sent_len, len, MSG_NOSIGNAL | MSG_DONTWAIT);
        if( s_ret <= 0 )
        {
            if( errno == EAGAIN || errno == EINTR) {
                continue;
            }
            else {
                RPC_LOG(RPC_LOG_LEV::ERROR, "send error! %s", strerror(errno));
                is_connected = false;
                return RpcStatus::RPC_SEND_FAIL;
            }
        }
        sent_len += s_ret;
        if (sent_len == total_len) {
            break;
        }

    }

    total_len = out_data.length();
    sent_len = 0;
    while(true) {
        if (timeout > 0 &&
            std::time(nullptr) - begin_tm > timeout) {
            RPC_LOG(RPC_LOG_LEV::WARNING, "send data timeout!");
            return RpcStatus::RPC_SEND_TIMEOUT;
        }
        uint32_t len = total_len - sent_len;
        errno = 0;
        int s_ret = send(m_fd, out_data.c_str() + sent_len, len, MSG_NOSIGNAL | MSG_DONTWAIT);
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
