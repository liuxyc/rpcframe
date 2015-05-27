/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCCLIENT
#define RPCFRAME_RPCCLIENT
#include <utility>
#include <map>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "Queue.h"
#include "RpcPackage.h"


namespace rpcframe
{

class RpcEventLooper;

class RpcClientCallBack 
{
public:
    enum RpcCBStatus {
        RPC_OK,
        RPC_SEND_FAIL,
        RPC_DISCONNECTED,
    };
    RpcClientCallBack(){};
    virtual ~RpcClientCallBack(){};

    virtual void callback(const RpcCBStatus status, const std::string &response_data) = 0;

    std::string getType() {
        return m_type_mark;
    }

protected:
    std::string m_type_mark;
    
};

class RpcClientBlocker: public RpcClientCallBack 
{
public:
    RpcClientBlocker()
    : m_done(false)
    { m_type_mark = "blocker"; };
    virtual ~RpcClientBlocker() {};
    std::pair<RpcCBStatus, std::string> wait() {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_resp_data = "";
        m_cb_st = RPC_OK;
        if (!m_done) {
            m_cv.wait(lk);
        }
        return std::make_pair(m_cb_st, std::string(m_resp_data));
    }

    virtual void callback(const RpcCBStatus status, const std::string &response_data) {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_resp_data = response_data;
        m_cb_st = status;
        m_done = true;
        m_cv.notify_all();
    }

    std::string getRespData() {
        return m_resp_data;
    }

    RpcCBStatus getCBstatus() {
        return m_cb_st;
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_done;
    std::string m_resp_data;
    RpcCBStatus m_cb_st;
    
};


class RpcClientConfig
{
public:
    RpcClientConfig(std::pair<const char *, int> &);
    ~RpcClientConfig();
    void setThreadNum(uint32_t thread_num);
    uint32_t getThreadNum();

    uint32_t m_thread_num;
    std::string m_hostname;
    int m_port;
};

class server_resp_pkg;

//NOTICE: start/stop RpcClient is heavy, keep the instant as long as possiable
class RpcClient
{
public:
    RpcClient(rpcframe::RpcClientConfig &cfg, const std::string &service_name);
    ~RpcClient();
    bool call(const std::string &method_name, const std::string &request_data, std::string &response_data, int timeout);
    bool async_call(const std::string &method_name, const std::string &request_data, int timeout, RpcClientCallBack *cb_obj);

    rpcframe::RpcClientConfig m_cfg;
    int m_connect_timeout;
private:

    bool m_isConnected;
    int m_fd;
    std::string m_servicename;
    RpcEventLooper *m_ev;
    std::mutex m_mutex;

    std::vector<std::thread *> m_thread_vec;
    Queue<server_resp_pkg *> m_response_q;

    
};

};
#endif
