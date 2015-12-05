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
        RPC_DISCONNECTED,
        RPC_SEND_TIMEOUT,
        RPC_CB_OK,
        RPC_CB_TIMEOUT = 5,
        RPC_SRV_NOTFOUND,
        RPC_METHOD_NOTFOUND,
        RPC_SERVER_OK,
        RPC_SERVER_NONE,
        RPC_SERVER_FAIL,
    };
class RpcServerConfig
{
public:
    explicit RpcServerConfig(std::pair<const char *, int> &);
    ~RpcServerConfig();
    void setThreadNum(uint32_t thread_num);
    uint32_t getThreadNum();
    void setMaxConnection(uint32_t max_conn_num);
    void enableHttp(int port, int thread_num);
    void disableHttp();
    int getHttpPort();

    uint32_t m_thread_num;
    std::string m_hostname;
    int m_port;
    uint32_t m_max_conn_num;
    int m_http_port;
    int m_http_thread_num;
};
};
#endif
