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

namespace rpcframe
{

class RpcServerConn;
class RpcWorker;
class RpcServerConnWorker;
class IService;
class RpcHttpServer;
class RpcStatusService;
class RpcInnerResp;
class RpcServerConfig;

typedef std::unordered_map<std::string, IService *> ServiceMap;

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

    void DecConnCount();
    void IncConnCount();
    uint64_t GetConnCount();
    void calcReqQTime(uint64_t);
    void calcRespQTime(uint64_t);
    void calcCallTime(uint64_t);
    void IncRejectedConn();
    void IncReqInQFail();
    void IncRespInQFail();
    const RpcServerConfig *getConfig();
    std::vector<RpcServerConnWorker *> &getConnWorker();

private:
    bool startListen();
    RpcServerConfig &m_cfg;
    std::vector<RpcWorker *> m_worker_vec;
    ServiceMap m_service_map;
    ReqQueue m_request_q;
    RespQueue m_response_q;
    int m_listen_socket;
    std::atomic<bool> m_stop;
    std::atomic<uint64_t> m_conn_num;
    RpcHttpServer *m_http_server;
    std::vector<RpcServerConnWorker *> m_connworker;

    uint64_t avg_req_wait_time;
    uint64_t avg_resp_wait_time;
    uint64_t avg_call_time;
    uint64_t max_call_time;
    uint64_t total_req_num;
    uint64_t total_resp_num;
    uint64_t total_call_num;
    uint64_t rejected_conn;
    uint64_t req_inqueue_fail;
    uint64_t resp_inqueue_fail;
    SpinLock m_stat_lock;
};

};
#endif
