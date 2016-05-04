/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include <set>
#include <string>
#include <stdint.h>
#include <atomic>

namespace rpcframe
{

class RpcServerConfig
{
public:
    explicit RpcServerConfig(std::pair<const char *, int> &);
    ~RpcServerConfig();
    void setThreadNum(uint32_t thread_num);
    uint32_t getThreadNum();
    void setConnThreadNum(uint16_t thread_num);
    uint16_t getConnThreadNum();
    void setMaxConnection(uint32_t max_conn_num);
    void setMaxReqPkgSize(uint32_t max_req_size);
    void setMaxReqQSize(uint32_t max_req_qsize);
    void enableHttp(int port, int thread_num);
    void disableHttp();
    int getHttpPort();

    uint32_t m_thread_num;
    std::string m_hostname;
    int m_port;
    std::atomic<uint32_t> m_max_conn_num;
    std::atomic<uint32_t> m_max_req_size;
    int m_http_port;
    int m_http_thread_num;
    std::atomic<uint32_t> m_max_req_qsize;
    uint16_t m_conn_thread_num;
};

};
