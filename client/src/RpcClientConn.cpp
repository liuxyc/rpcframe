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
#include <uuid/uuid.h>

#include "RpcClientConn.h"
#include "rpc.pb.h"

namespace rpcframe
{

RpcClientConn::RpcClientConn(int fd)
: m_fd(fd)
, m_cur_left_len(0)
, m_cur_pkg_size(0)
, m_rpk(NULL)
, is_connected(true)
{
    m_seqid = std::to_string(std::time(nullptr)) + "_";
    m_seqid += std::to_string(fd);
}

RpcClientConn::~RpcClientConn()
{
    printf("close fd %d\n", m_fd);
    ::close(m_fd);
    if (m_rpk != NULL)
        delete m_rpk;
}

void RpcClientConn::reset()
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

int RpcClientConn::getFd()
{
    return m_fd;
}

bool RpcClientConn::readPkgLen(uint32_t &pkg_len)
{
    if (!is_connected) {
        printf("connection already disconnected\n");
        return false;
    }
    char data[4]; 
    int rev_size = recv(m_fd, data, 4, 0);  
    if (rev_size <= 0) {
        if (rev_size != 0) {
            printf("recv pkg len error %s\n", strerror(errno));
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
    pkg_len = ntohl(*(int *)data);
    return true;
}

int RpcClientConn::readPkgData()
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
            printf("recv pkg error\n");
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
        printf(" half pkg got %d need %d more\n", rev_size, m_cur_left_len);
        return -1;
    }
}

pkg_ret_t RpcClientConn::getResponse()
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
        m_rpk = new server_resp_pkg(m_cur_pkg_size);
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
        server_resp_pkg *p_rpk = m_rpk;
        m_rpk = NULL;
        return pkg_ret_t(0, p_rpk);
    }
}

std::string RpcClientConn::genRequestId() {
    char uuid_buf[37];
    uuid_t uid;
    uuid_generate(uid);
    uuid_unparse(uid, uuid_buf);
    return std::string("rpcframe::id_" + std::to_string(m_fd) + "_" + std::string(uuid_buf));
}

RpcStatus RpcClientConn::sendReq(const std::string &service_name, const std::string &method_name, const std::string &request_data, const std::string &reqid, bool is_oneway, uint32_t timeout) {

    RpcInnerReq req;
    req.set_service_name(service_name);
    req.set_methond_name(method_name);

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
            printf("send data len timeout!\n");
            return RpcStatus::RPC_SEND_TIMEOUT;
        }
        uint32_t len = total_len - sent_len;
        int s_ret = send(m_fd, ((char *)&nlen) + sent_len, len, MSG_NOSIGNAL | MSG_DONTWAIT);
        if( s_ret <= 0 )
        {
            if( errno != EAGAIN) {
                printf("send error! %s\n", strerror(errno));
                is_connected = false;
                return RpcStatus::RPC_SEND_FAIL;
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

    total_len = out_data.length();
    sent_len = 0;
    while(true) {
        if (timeout > 0 &&
            std::time(nullptr) - begin_tm > timeout) {
            printf("send data timeout!\n");
            return RpcStatus::RPC_SEND_TIMEOUT;
        }
        uint32_t len = total_len - sent_len;
        int s_ret = send(m_fd, out_data.c_str() + sent_len, len, MSG_NOSIGNAL | MSG_DONTWAIT);
        if( s_ret <= 0)
        {
            if( errno != EAGAIN) {
                printf("send data error! %s, %u, %u, %d %u\n", strerror(errno), total_len, sent_len, s_ret, len);
                is_connected = false;
                return RpcStatus::RPC_SEND_FAIL;
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

    return RpcStatus::RPC_SEND_OK;
}

};
