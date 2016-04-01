/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCSERVERIMPL
#define RPCFRAME_RPCSERVERIMPL
#include <unordered_map>
#include <vector>
#include <atomic>
#include <thread>

#include "Queue.h"
#include "SpinLock.h"
#include "RpcDefs.h"
#include "RpcPackage.h"
#include "RpcServerConfig.h"

namespace rpcframe
{

class RpcServerConn;
class RpcWorker;
class IService;
class RpcHttpServer;
class RpcStatusService;
class RpcInnerResp;
class RpcServerConfig;

typedef std::unordered_map<std::string, IService *> ServiceMap;

class RpcMethodStatus 
{
public:
    uint64_t total_call_nums = 0;
    uint64_t timeout_call_nums = 0;
    uint64_t avg_call_time = 0;  // unit: ms
    uint64_t longest_call_time = 0;   // unit: ms
};

typedef std::unordered_map<std::string, RpcMethodStatus> MethodStatusMap;

class RpcServerImpl
{
    friend RpcStatusService;
public:
    explicit RpcServerImpl(RpcServerConfig &cfg);
    RpcServerImpl &operator=(const RpcServerConfig &cfg) = delete;
    ~RpcServerImpl();

    /**
     * @brief add IService implement to RpcServerImpl
     *
     * @param name  Service name
     * @param       Service instance
     *
     * @return 
     */
    bool addService(const std::string &name, IService *);
    bool addWorkers(const uint32_t numbers);
    bool removeWorkers(const uint32_t numbers);
    IService *getService(const std::string &name);

    /**
     * @brief start RpcServer, this method will block
     *
     * @return 
     */
    bool start();
    void stop();

    void setSocketKeepAlive(int fd);
    void removeConnection(int fd);
    void addConnection(int fd, RpcServerConn *conn);
    RpcServerConn *getConnection(int fd);
    void pushResp(std::string seqid, RpcInnerResp &resp);
    void calcReqQTime(uint64_t);
    void calcRespQTime(uint64_t);
    void calcCallTime(uint64_t);
    const RpcServerConfig *getConfig();

private:
    bool startListen();
    void onDataOut(const int fd);
    bool onDataOutEvent();
    void onAccept();
    void onDataIn(const int fd);

    RpcServerConfig m_cfg;
    std::vector<RpcWorker *> m_worker_vec;
    ServiceMap m_service_map;
    MethodStatusMap m_methodstatus_map;
    std::unordered_map<int, RpcServerConn *> m_conn_map;
    std::unordered_map<std::string, RpcServerConn *> m_conn_set;
    uint32_t m_seqid;
    ReqQueue m_request_q;
    RespQueue m_response_q;
    Queue<std::string> m_resp_conn_q;
    std::mutex m_mutex;
    int m_epoll_fd;
    int m_listen_socket;
    int m_resp_ev_fd;
    std::atomic<bool> m_stop;
    RpcHttpServer *m_http_server;

    uint64_t avg_req_wait_time;
    uint64_t avg_resp_wait_time;
    uint64_t avg_call_time;
    uint64_t max_call_time;
    uint64_t total_req_num;
    uint64_t total_resp_num;
    uint64_t total_call_num;
    SpinLock m_stat_lock;
    
};

};
#endif
