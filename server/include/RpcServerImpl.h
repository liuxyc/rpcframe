/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
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
#include "RpcHttpServer.h"

namespace rpcframe
{

class RpcServerConn;
class RpcServerConnWorker;
class IService;
class RpcHttpServer;
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
    template <typename T> 
    bool addService(const std::string &name, IService *p_service)
    {
        IService *pService = p_service;
        for(auto &worker:m_worker_vec) {
            if(p_service == nullptr) {
              pService = new T();
            }
            worker->addService(name, pService, (p_service == nullptr));
        }
        m_http_server->addService(name, pService, (p_service == nullptr));
        return true;
    }

    bool addWorkers(const uint32_t numbers);
    bool removeWorkers(const uint32_t numbers);

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

    RpcHttpServer *m_http_server;
private:
    bool startListen();
    RpcServerConfig &m_cfg;
    std::vector<RpcWorker *> m_worker_vec;
    //ServiceMap m_service_map;
    ReqQueue m_request_q;
    RespQueue m_response_q;
    int m_listen_socket;
    std::atomic<bool> m_stop;
    std::atomic<uint64_t> m_conn_num;
    std::vector<RpcServerConnWorker *> m_connworker;
    IService *m_statusSrv;

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
