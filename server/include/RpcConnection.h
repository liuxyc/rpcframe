/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCCONNECTION
#define RPCFRAME_RPCCONNECTION
#include "RpcPackage.h"
#include "Queue.h"
#include <mutex>
#include <ctime>
#include <set>

namespace rpcframe {

typedef std::pair<int, request_pkg *> pkg_ret_t;

class RpcConnection
{
public:
    RpcConnection(int fd, uint32_t seqid);
    ~RpcConnection();

    pkg_ret_t getRequest();
    void reset();

    int sendResponse();
    int getFd();
    bool isSending();

    Queue<response_pkg *> m_response_q;
    std::string m_seqid;

private:
    bool readPkgLen(uint32_t &pkg_len);
    int readPkgData();
    int sendPkgLen();
    int sendData();
    int m_fd;
    uint32_t m_cur_left_len;
    uint32_t m_cur_pkg_size;
    request_pkg *m_rpk;
    bool is_connected;
    std::mutex m_mutex;
    uint32_t m_sent_len;
    response_pkg *m_sent_pkg;
    
};
};

#endif
