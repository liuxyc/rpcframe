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

    uint32_t m_thread_num;
    std::string m_hostname;
    int m_port;
};

class RpcServer
{
public:
    RpcServer(rpcframe::RpcServerConfig &cfg);
    ~RpcServer();
    bool addService(const std::string &name, rpcframe::IService *);
    rpcframe::IService *getService(const std::string &name);

    bool start();
    void stop();

    bool hasConnection(int fd);
    void removeConnection(int fd);
    void addConnection(int fd, rpcframe::RpcConnection *conn);
    RpcConnection *getConnection(int fd);
    void pushResp(std::string seqid, rpcframe::response_pkg *resp_pkg);

private:
    rpcframe::RpcServerConfig m_cfg;
    std::vector<std::thread *> m_thread_vec;
    std::vector<RpcWorker *> m_worker_vec;
    rpcframe::ServiceMap m_service_map;
    std::unordered_map<int, rpcframe::RpcConnection *> m_conn_map;
    std::unordered_map<std::string, rpcframe::RpcConnection *> m_conn_set;
    uint32_t m_seqid;

    rpcframe::Queue<rpcframe::request_pkg *> m_request_q;
    rpcframe::Queue<rpcframe::response_pkg *> m_response_q;
    std::thread *m_resp_th;
    std::mutex m_mutex;
    int m_resp_ev_fd;
    std::atomic<bool> m_stop;
    
};

};
#endif
