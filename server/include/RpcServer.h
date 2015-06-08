/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCSERVER
#define RPCFRAME_RPCSERVER
#include <utility>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <ctime>
#include <vector>

#include "Queue.h"
#include "IService.h"
#include "RpcPackage.h"
#include "RpcConnection.h"
#include "RpcWorker.h"

namespace rpcframe
{

class RpcServerConfig
{
public:
    RpcServerConfig(std::pair<const char *, int> &);
    ~RpcServerConfig();
    void setThreadNum(uint32_t thread_num);
    uint32_t getThreadNum();
    void setMaxConnection(uint32_t max_conn_num);

    uint32_t m_thread_num;
    std::string m_hostname;
    int m_port;
    uint32_t m_max_conn_num;
};

class RpcServer
{
public:
    RpcServer(RpcServerConfig &cfg);
    ~RpcServer();
    bool addService(const std::string &name, IService *);
    IService *getService(const std::string &name);

    bool start();
    void stop();

    void setSocketKeepAlive(int fd);
    bool hasConnection(int fd);
    void removeConnection(int fd, int epoll_fd);
    void addConnection(int fd, RpcConnection *conn);
    RpcConnection *getConnection(int fd);
    void pushResp(std::string seqid, response_pkg *resp_pkg);

private:
    RpcServerConfig m_cfg;
    std::vector<std::thread *> m_thread_vec;
    std::vector<RpcWorker *> m_worker_vec;
    ServiceMap m_service_map;
    std::unordered_map<int, RpcConnection *> m_conn_map;
    std::unordered_map<std::string, RpcConnection *> m_conn_set;
    uint32_t m_seqid;

    Queue<request_pkg *> m_request_q;
    Queue<response_pkg *> m_response_q;
    Queue<std::string> m_resp_conn_q;
    //std::thread *m_resp_th;
    std::mutex m_mutex;
    int m_resp_ev_fd;
    std::atomic<bool> m_stop;
    
};

};
#endif
