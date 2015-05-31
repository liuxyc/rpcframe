/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_EVENTLOOPER
#define RPCFRAME_EVENTLOOPER
#include <utility>
#include <unordered_map>
#include <map>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include "Queue.h"
#include "RpcClientEnum.h"

namespace rpcframe
{

class RpcClient;
class RpcClientCallBack;
class RpcClientConn;
class RpcClientWorker;
class server_resp_pkg;

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
    void dealTimeoutCb();
    Queue<server_resp_pkg *> m_response_q;

private:
    void addConnection();
    void removeConnection();
    bool connect();
    int setNoBlocking(int fd);
    int noBlockConnect(int sockfd, const char* ip,int port,int timeout);
    RpcClient *m_client;
    std::atomic<bool> m_stop;
    int m_epoll_fd;
    int m_fd;
    RpcClientConn *m_conn;
    std::mutex m_mutex;
    std::unordered_map<std::string, RpcClientCallBack *> m_cb_map;
    RpcClientWorker *m_worker;
    std::thread *m_worker_th;
    //FIXME:the resolution of std::time_t is not enough for timeout
    //there will be >1000 call in one second, they will have the same time_t
    std::map<std::string, std::string> m_cb_timer_map;
    uint32_t m_req_seqid;

    const uint32_t MAX_REQ_LIMIT_BYTE;
    
};

};
#endif
