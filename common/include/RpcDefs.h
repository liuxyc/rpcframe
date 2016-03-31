/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCDEFS
#define RPCFRAME_RPCDEFS
#include <utility>
#include <string>

namespace rpcframe
{
    enum class RpcStatus {
        RPC_SEND_OK = 0,
        RPC_SEND_FAIL,
        RPC_REQ_TOO_LARGE,
        RPC_DISCONNECTED,
        RPC_SEND_TIMEOUT,
        RPC_CB_TIMEOUT,
        RPC_SRV_NOTFOUND = 5,
        RPC_METHOD_NOTFOUND,
        RPC_SERVER_OK,
        RPC_SERVER_NONE,
        RPC_SERVER_FAIL,
        RPC_MALLOC_PKG_FAIL,
    };
class RpcServerConfig
{
public:
    explicit RpcServerConfig(std::pair<const char *, int> &);
    ~RpcServerConfig();
    void setThreadNum(uint32_t thread_num);
    uint32_t getThreadNum();
    void setMaxConnection(uint32_t max_conn_num);
    void setMaxReqPkgSize(uint32_t max_req_size);
    void enableHttp(int port, int thread_num);
    void disableHttp();
    int getHttpPort();

    uint32_t m_thread_num;
    std::string m_hostname;
    int m_port;
    uint32_t m_max_conn_num;
    uint32_t m_max_req_size;
    int m_http_port;
    int m_http_thread_num;
};
};
#endif
