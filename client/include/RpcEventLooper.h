/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include <utility>
#include <unordered_map>
#include <map>
#include <atomic>
#include <memory>
#include <thread>
#include <ctime>

#include "Queue.h"
#include "RpcDefs.h"
#include "RpcPackage.h"
#include "ThreadPool.h"
#include "Epoll.h"

namespace rpcframe
{

class RpcClient;
class RpcClientCallBack;
class RpcClientConn;
class RpcClientWorker;


typedef std::shared_ptr<RpcClientCallBack> RpcCBPtr;

class RpcEventLooper
{
public:
    RpcEventLooper(RpcClient *client, int thread_num);
    ~RpcEventLooper();
    void stop();
    void run();
    RpcStatus sendReq(const std::string &service_name, const std::string &method_name, const RawData &request_data, std::shared_ptr<RpcClientCallBack> cb_obj, std::string &req_id);
    std::shared_ptr<RpcClientCallBack> getCb(const std::string &req_id);
    void removeCb(const std::string &req_id);
    void dealTimeoutCb();
    void waitAllCBDone(uint32_t timeout);
    void addConnection(int fd, RpcClientConn *data);
    void removeConnection(int fd, RpcClientConn *conn);
    void refreshEndpoints();

    RpcEventLooper(const RpcEventLooper &) = delete;
    RpcEventLooper &operator=(const RpcEventLooper &) = delete;
private:
    void removeAllConnections();
    bool connect(const Endpoint &ep);
    static int setNoBlocking(int fd);
    static int noBlockConnect(int sockfd, const char* hostname,int port,int timeout);
    void GenReqID(std::string &id, RpcClientConn *conn);
    RpcClientConn *tryGetAvaliableConn();
    RpcClient *m_client;
    std::atomic<bool> m_stop;
    std::vector<RpcClientConn *> m_conn_list;
    std::map<Endpoint, RpcClientConn *> m_ep_conn_map;
    std::mutex m_mutex;
    std::unordered_map<std::string, RpcCBPtr> m_id_cb_map;
    std::unordered_map<RpcClientConn*, std::vector<RpcCBPtr> > m_conn_cb_map;
    std::multimap<std::time_t, std::string> m_cb_timer_map;
    uint32_t m_req_seqid;
    std::string m_host_ip;
    int m_thread_num;
    ThreadPool<RespPkgPtr, RpcClientWorker> *m_thread_pool;
    Epoll m_epoll;
    
};

};
