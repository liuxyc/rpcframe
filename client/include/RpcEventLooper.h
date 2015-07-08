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
#include <thread>
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
    RpcEventLooper(RpcClient *client);
    ~RpcEventLooper();
    void stop();
    void run();
    RpcStatus sendReq(const std::string &service_name, const std::string &method_name, const std::string &request_data, RpcClientCallBack *cb_obj, std::string &req_id);
    RpcClientCallBack *getCb(const std::string &req_id);
    void removeCb(const std::string &req_id);
    void timeoutCb(const std::string &req_id, bool is_lock = true);
    void dealTimeoutCb();
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
    std::unordered_map<std::string, RpcClientCallBack *> m_cb_map;
    RpcClientWorker *m_worker;
    std::thread *m_worker_th;
    std::map<std::string, std::string> m_cb_timer_map;
    uint32_t m_req_seqid;
    std::string m_host_ip;

    const uint32_t MAX_REQ_LIMIT_BYTE;

    
};

};
#endif
