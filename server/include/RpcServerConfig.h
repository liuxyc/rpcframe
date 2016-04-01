/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCSERVER_CONFIG
#define RPCFRAME_RPCSERVER_CONFIG

#include <set>
#include <string>
#include <stdint.h>

namespace rpcframe
{
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
