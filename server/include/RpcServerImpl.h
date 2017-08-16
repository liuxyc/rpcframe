/*
 * Copyright (c) 2015-2017, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include <unordered_map>
#include <map>
#include <vector>
#include <atomic>
#include <thread>

#include "Queue.h"
#include "SpinLock.h"
#include "RpcDefs.h"
#include "RpcPackage.h"
#include "RpcWorker.h"
#include "RpcServerConfig.h"
//#include "RpcHttpServer.h"
#include "ThreadPool.h"

namespace rpcframe
{

class RpcServerConn;
class RpcServerConnWorker;
class IService;
//class RpcHttpServer;
class RpcStatusService;
class RpcInnerResp;

//TODO: need a ServiceList to keep service names, it will used in statsic data collection
//typedef std::map<std::pair<std::string, void *>, IService *> ServiceMap;

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
    bool addService(const std::string &name, IService *p_service)
    {
        std::vector<RpcWorker *> real_workers;
        m_worker_thread_pool->getWorkers(real_workers);
        for(auto w: real_workers) {
            w->addService(name, p_service, false);
        }
        //m_http_server->addService(name, p_service, false);
        return true;
    }

    bool pushReqToWorkers(ReqPkgPtr);

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
    std::vector<RpcServerConnWorker *> &getRpcConnWorker();

    //RpcHttpServer *m_http_server;
private:
    int startListen(int port);
    int m_rpclisten;
    int m_httplisten;
    RpcServerConfig &m_cfg;
    //ServiceMap m_service_map;
    RespQueue m_response_q;
    std::atomic<bool> m_stop;
    std::atomic<uint64_t> m_conn_num;
    std::vector<RpcServerConnWorker *> m_rpcconnworker;
    std::vector<RpcServerConnWorker *> m_httpconnworker;
    ThreadPool<ReqPkgPtr, RpcWorker> *m_worker_thread_pool;
    IService *m_statusSrv;
    IService *m_service;

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
