/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCSERVERCONN
#define RPCFRAME_RPCSERVERCONN
#include <mutex>
#include <memory>

#include "RpcPackage.h"
#include "Queue.h"

namespace rpcframe {

typedef std::pair<int, std::shared_ptr<request_pkg> > pkg_ret_t;

class RpcServerImpl;

class RpcServerConn
{
public:
    RpcServerConn(int fd, const char *seqid, RpcServerImpl *server);
    ~RpcServerConn();

    pkg_ret_t getRequest();
    void reset();

    PkgIOStatus sendResponse();
    int getFd() const ;
    bool isSending() const ;

    RespQueue m_response_q;
    std::string m_seqid;

private:
    bool readPkgLen(uint32_t &pkg_len);
    PkgIOStatus readPkgData();
    PkgIOStatus sendData();
    int m_fd;
    uint32_t m_cur_left_len;
    uint32_t m_cur_pkg_size;
    request_pkg *m_rpk;
    bool is_connected;
    std::mutex m_mutex;
    uint32_t m_sent_len;
    RespPkgPtr m_sent_pkg;
    const uint32_t MAX_REQ_LIMIT_BYTE;
    RpcServerImpl *m_server;
    
};
};

#endif
