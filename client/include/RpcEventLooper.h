/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_EVENTLOOPER
#define RPCFRAME_EVENTLOOPER
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

namespace rpcframe
{

class RpcClient;
class RpcClientCallBack;
class RpcClientConn;
class RpcClientWorker;

class RpcEventLooper
{
public:
    RpcEventLooper(RpcClient *client, int thread_num);
    ~RpcEventLooper();
    void stop();
    void run();
    RpcStatus sendReq(const std::string &service_name, const std::string &method_name, const std::string &request_data, std::shared_ptr<RpcClientCallBack> cb_obj, std::string &req_id);
    std::shared_ptr<RpcClientCallBack> getCb(const std::string &req_id);
    void removeCb(const std::string &req_id);
    void dealTimeoutCb();
    void waitAllCBDone(uint32_t timeout);
    RespQueue m_response_q;

    RpcEventLooper(const RpcEventLooper &) = delete;
    RpcEventLooper &operator=(const RpcEventLooper &) = delete;
private:
    void addConnection();
    void removeConnection();
    bool connect();
    static int setNoBlocking(int fd);
    static int noBlockConnect(int sockfd, const char* hostname,int port,int timeout);
    RpcClient *m_client;
    std::atomic<bool> m_stop;
    int m_epoll_fd;
    int m_fd;
    RpcClientConn *m_conn;
    std::mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<RpcClientCallBack> > m_cb_map;
    std::multimap<std::time_t, std::string> m_cb_timer_map;
    uint32_t m_req_seqid;
    std::string m_host_ip;
    int m_thread_num;
    std::vector<std::thread *> m_thread_vec;
    std::vector<RpcClientWorker *> m_worker_vec;


    
};

};
#endif
