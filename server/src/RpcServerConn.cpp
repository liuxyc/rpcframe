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

namespace rpcframe
{

RpcServerConn::RpcServerConn(int fd, uint32_t seqid)
: m_fd(fd)
, m_cur_left_len(0)
, m_cur_pkg_size(0)
, m_rpk(nullptr)
, is_connected(true)
, m_sent_len(0)
, m_sent_pkg(nullptr)
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
    while(m_response_q.size() != 0) {
        response_pkg *pkg = nullptr;
        if (m_response_q.pop(pkg, 0)) {
            delete pkg;
        }
    }
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
    int rev_size = recv(m_fd, (char *)&data, 4, 0);  
    if (rev_size <= 0) {
        if (rev_size != 0) {
            RPC_LOG(RPC_LOG_LEV::ERROR, "try recv pkg len error %s", strerror(errno));
        }
        if( errno == EAGAIN || errno == EINTR) {
        }
        else {
            return false;
        }
    }
    if( rev_size < 4 )  
    {  
        RPC_LOG(RPC_LOG_LEV::ERROR, "recv data too small %d, close connection: %d", rev_size, m_fd);
        return false;
    }  
    pkg_len = ntohl(data);
    return true;
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
    int rev_size = recv(m_fd, m_rpk->data + (m_cur_pkg_size - m_cur_left_len), m_cur_left_len, 0);  
    if (rev_size <= 0) {
        if( errno == EAGAIN || errno == EINTR) {
            return PkgIOStatus::PARTIAL;
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
        m_cur_left_len = m_cur_left_len - rev_size;
        //RPC_LOG(RPC_LOG_LEV::DEBUG, " half pkg got %d need %d more", rev_size, m_cur_left_len);
        return PkgIOStatus::PARTIAL;
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
        //TODO: may produce many memory fragment here
        m_rpk = new request_pkg(m_cur_pkg_size, m_seqid);
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

int RpcServerConn::sendPkgLen()
{
    uint32_t pkg_len = m_sent_pkg->data_len;
    uint32_t nlen = htonl(pkg_len);
    std::time_t begin_tm = std::time(nullptr);
    uint32_t total_len = sizeof(nlen);
    uint32_t sent_len = 0;
    //NOTE:if send resp len takes over 1 second, the network is too slow, 
    //to avoid take too much server resource, we disconnet!
    while(true) {
        if ( std::time(nullptr) - begin_tm > 1) {
            delete m_sent_pkg;
            m_sent_pkg = nullptr;
            m_sent_len = 0;
            RPC_LOG(RPC_LOG_LEV::WARNING, "send data len timeout!");
            return -1;
        }
        uint32_t len = total_len - sent_len;
        errno = 0;
        int s_ret = send(m_fd, ((char *)&nlen) + sent_len, len, MSG_NOSIGNAL | MSG_DONTWAIT);
        if( s_ret <= 0 )
        {
            if( errno == EAGAIN || errno == EINTR) {
                continue;
            }
            else {
                RPC_LOG(RPC_LOG_LEV::ERROR, "send error! %s", strerror(errno));
                is_connected = false;
                delete m_sent_pkg;
                m_sent_pkg = nullptr;
                m_sent_len = 0;
                return -1;
            }
        }
        sent_len += s_ret;
        if (sent_len == total_len) {
            break;
        }

    }
    return 0;
}

PkgIOStatus RpcServerConn::sendData()
{
    int slen = send(m_fd, 
                    m_sent_pkg->data + m_sent_len, 
                    m_sent_pkg->data_len - m_sent_len, 
                    MSG_NOSIGNAL | MSG_DONTWAIT);  
    if (slen <= 0) {
        if (slen == 0 || errno == EPIPE) {
            delete m_sent_pkg;
            RPC_LOG(RPC_LOG_LEV::WARNING, "peer closed");
            return PkgIOStatus::FAIL;
        }
        if( errno != EAGAIN && errno != EINTR) {
            RPC_LOG(RPC_LOG_LEV::ERROR, "send data error! %s", strerror(errno));
            delete m_sent_pkg;
            m_sent_pkg = nullptr;
            m_sent_len = 0;
            return PkgIOStatus::FAIL;
        }
    }
    m_sent_len += slen;
    if (m_sent_pkg->data_len == m_sent_len) {
        //RPC_LOG(RPC_LOG_LEV::DEBUG, "full send %d", m_sent_len);
        delete m_sent_pkg;
        m_sent_pkg = nullptr;
        m_sent_len = 0;
        return PkgIOStatus::FULL;
    }
    else {
        //RPC_LOG(RPC_LOG_LEV::DEBUG, "left send %d", m_sent_pkg->data_len - m_sent_len);
        return PkgIOStatus::PARTIAL;
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
        response_pkg *pkg = nullptr;
        if (m_response_q.pop(pkg, 0)) {
            m_sent_len = 0;
            m_sent_pkg = pkg;
            if (sendPkgLen() == -1) {
                RPC_LOG(RPC_LOG_LEV::ERROR, "send pkg len failed");
                return PkgIOStatus::FAIL;
            }
            //RPC_LOG(RPC_LOG_LEV::DEBUG, "send len success");
            return sendData();
        }
        return PkgIOStatus::PARTIAL;
    }
}

bool RpcServerConn::isSending() const {
    return (m_sent_pkg != nullptr);
}

};
