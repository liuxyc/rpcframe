/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#pragma once
#include <atomic>
#include <thread>
#include <map>

#include "Queue.h"
#include "RpcPackage.h"
#include "RpcRespBroker.h"

struct mg_connection;
namespace rpcframe
{

class RpcServerImpl;
class IService;

class RpcWorker
{
  public:
    class ServiceBlock 
    {
      public:
        ServiceBlock()
        : ServiceBlock(nullptr, false)
        {};
        ServiceBlock(IService *p, bool o)
        : pSrv(p)
        , owner(o)
        {};
        IService *pSrv;
        bool owner;
    };
    RpcWorker(ReqQueue *workqueue, RpcServerImpl *server);
    RpcWorker &operator=(const RpcWorker &worker) = delete;
    RpcWorker(const RpcWorker &worker) = delete;
    ~RpcWorker();
    void stop();
    void run();
    void addService(const std::string &name, IService *service, bool owner);
    std::map<std::string, ServiceBlock> m_srvmap;
  private:
    void pushResponse(IRpcRespBrokerPtr &rpcbroker, std::string &connid, int type, RpcServerConnWorker *connworker);
    ReqQueue *m_work_q;
    RpcServerImpl *m_server;
    IService *m_service;
    std::atomic<bool> m_stop;
    std::thread *m_thread;

};

};
