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
#include "IService.h"
#include "RpcDefs.h"

namespace rpcframe
{

class RpcServerConn;
class request_pkg;
class response_pkg;
class RpcWorker;
class IService;
class RpcHttpServer;

typedef std::unordered_map<std::string, IService *> ServiceMap;

class RpcServerImpl
{
public:
    explicit RpcServerImpl(RpcServerConfig &cfg);
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
    void pushResp(std::string seqid, response_pkg *resp_pkg);
    bool pushReq(request_pkg *req_pkg);

private:
    bool startListen();
    void onDataOut(const int fd);
    bool onDataOutEvent();
    void onAccept();
    void onDataIn(const int fd);

    RpcServerConfig m_cfg;
    std::vector<std::thread *> m_thread_vec;
    std::vector<RpcWorker *> m_worker_vec;
    ServiceMap m_service_map;
    std::unordered_map<int, RpcServerConn *> m_conn_map;
    std::unordered_map<std::string, RpcServerConn *> m_conn_set;
    uint32_t m_seqid;
    Queue<request_pkg *> m_request_q;
    Queue<response_pkg *> m_response_q;
    Queue<std::string> m_resp_conn_q;
    std::mutex m_mutex;
    int m_epoll_fd;
    int m_listen_socket;
    int m_resp_ev_fd;
    std::atomic<bool> m_stop;
    RpcHttpServer *m_http_server;
    
};

};
#endif
