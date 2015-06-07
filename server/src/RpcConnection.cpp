/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include <errno.h>
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <fcntl.h>  
#include <string.h>
#include <unistd.h>
#include <chrono>

#include "RpcConnection.h"
namespace rpcframe
{

RpcConnection::RpcConnection(int fd, uint32_t seqid)
: m_fd(fd)
, m_cur_left_len(0)
, m_cur_pkg_size(0)
, m_rpk(NULL)
, is_connected(true)
, m_sent_len(0)
, m_sent_pkg(NULL)
{
    //generate a connection id, this id used for track and identify RpcConnection instance
    m_seqid = std::to_string(std::time(nullptr)) + "_";
    m_seqid += std::to_string(seqid) + "_";
    m_seqid += std::to_string(fd);
}

RpcConnection::~RpcConnection()
{
    printf("close fd %d\n", m_fd);
    close(m_fd);
    if (m_rpk != NULL)
        delete m_rpk;
    while(m_response_q.size() != 0) {
        response_pkg *pkg = NULL;
        if (m_response_q.pop(pkg, 0)) {
            delete pkg;
        }
    }
}

void RpcConnection::reset()
{
    std::lock_guard<std::mutex> lck(m_mutex);
    is_connected = false;
    m_cur_left_len = 0;
    m_cur_pkg_size = 0;
    if (m_rpk != NULL) {
        delete m_rpk;
    }
    m_rpk = NULL;
}

int RpcConnection::getFd()
{
    return m_fd;
}

bool RpcConnection::readPkgLen(uint32_t &pkg_len)
{
    if (!is_connected) {
        printf("connection already disconnected\n");
        return false;
    }
    int data = 0;
    int rev_size = recv(m_fd, (char *)&data, 4, 0);  
    if (rev_size <= 0) {
        if (rev_size != 0) {
            printf("try recv pkg len error %s\n", strerror(errno));
        }
        if (errno != EAGAIN) {
            return false;
        }
    }
    if( rev_size < 4 )  
    {  
        printf("recv data too small %d, close connection: %d\n", rev_size, m_fd);
        return false;
    }  
    pkg_len = ntohl(data);
    return true;
}

int RpcConnection::readPkgData()
{
    if (m_rpk == NULL) {
        printf("rpk is NULL\n");
        return -2;
    }
    if (!is_connected) {
        printf("connection already disconnected\n");
        return -2;
    }
    int rev_size = recv(m_fd, m_rpk->data + (m_cur_pkg_size - m_cur_left_len), m_cur_left_len, 0);  
    if (rev_size <= 0) {
        if (errno != EAGAIN) {
            printf("recv pkg error %s\n", strerror(errno));
            return -2;
        }
        else {
            return -1;
        }
    }
    if ((uint32_t)rev_size == m_cur_left_len) {
        //printf("got full pkg\n");
        m_cur_left_len = 0;
        m_cur_pkg_size = 0;
        return 0;
    }
    else {
        m_cur_left_len = m_cur_left_len - rev_size;
        //printf(" half pkg got %d need %d more\n", rev_size, m_cur_left_len);
        return -1;
    }
}

pkg_ret_t RpcConnection::getRequest()
{
    if (m_cur_pkg_size == 0) {
        //new pkg
        if (!readPkgLen(m_cur_pkg_size)) {
            return pkg_ret_t(-1, NULL);
        }
        if (m_cur_pkg_size == 0) {
            return pkg_ret_t(0, NULL);
        }
        //printf("pkg len is %d\n", m_cur_pkg_size);
        m_rpk = new request_pkg(m_cur_pkg_size, m_seqid);
        m_cur_left_len = m_cur_pkg_size;
    }
    int data_ret = readPkgData();
    if (data_ret == -2) {
        return pkg_ret_t(-1, NULL);
    }
    else if( data_ret == -1) {
        return pkg_ret_t(0, NULL);
    }
    else {
        request_pkg *p_rpk = m_rpk;
        m_rpk = NULL;
        return pkg_ret_t(0, p_rpk);
    }
}

int RpcConnection::sendPkgLen()
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
            m_sent_pkg = NULL;
            m_sent_len = 0;
            printf("send data len timeout!\n");
            return -1;
        }
        uint32_t len = total_len - sent_len;
        int s_ret = send(m_fd, ((char *)&nlen) + sent_len, len, MSG_NOSIGNAL | MSG_DONTWAIT);
        if( s_ret <= 0 )
        {
            if( errno != EAGAIN) {
                printf("send error! %s\n", strerror(errno));
                is_connected = false;
                delete m_sent_pkg;
                m_sent_pkg = NULL;
                m_sent_len = 0;
                return -1;
            }
            else {
                continue;
            }
        }
        sent_len += s_ret;
        if (sent_len == total_len) {
            break;
        }

    }
    return 0;
}

int RpcConnection::sendData()
{
    int slen = send(m_fd, m_sent_pkg->data + m_sent_len, m_sent_pkg->data_len - m_sent_len, MSG_NOSIGNAL | MSG_DONTWAIT);  
    if (slen <= 0) {
        if( errno != EAGAIN) {
            if (slen == 0 || errno == EPIPE) {
                printf("peer closed\n");
            }
            printf("send data error! %s\n", strerror(errno));
            delete m_sent_pkg;
            m_sent_pkg = NULL;
            m_sent_len = 0;
            return -1;
        }
    }
    m_sent_len += slen;
    if (m_sent_pkg->data_len == m_sent_len) {
        //printf("full send %d\n", m_sent_len);
        delete m_sent_pkg;
        m_sent_pkg = NULL;
        m_sent_len = 0;
        return 0;
    }
    else {
        //printf("left send %d\n", m_sent_pkg->data_len - m_sent_len);
        return -2;
    }
}

int RpcConnection::sendResponse()
{
    if (!is_connected) {
        printf("send connection already disconnected\n");
        return -1;
    }
    if (m_sent_pkg != NULL) {
        return sendData();
    }
    else {
        response_pkg *pkg = NULL;
        if (m_response_q.pop(pkg, 0)) {
            m_sent_len = 0;
            m_sent_pkg = pkg;
            if (sendPkgLen() == -1) {
                printf("send pkg len failed\n");
                return -1;
            }
            //printf("send len success\n");
            return sendData();
        }
        return -2;
    }
}

bool RpcConnection::isSending() {
    return (m_sent_pkg != NULL);
}

};
