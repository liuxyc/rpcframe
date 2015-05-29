/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCCLIENTBLOCKER
#define RPCFRAME_RPCCLIENTBLOCKER
#include <utility>
#include <map>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "Queue.h"
#include "RpcPackage.h"
#include "RpcClient.h"


namespace rpcframe
{


class RpcClientBlocker: public RpcClientCallBack 
{
public:
    RpcClientBlocker(int timeout)
    : RpcClientCallBack()
    , m_done(false)
    , m_timeout(timeout)
    { m_type_mark = "blocker"; };
    virtual ~RpcClientBlocker() {};
    std::pair<RpcCBStatus, std::string> wait() {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_resp_data = "";
        m_cb_st = RpcCBStatus::RPC_OK;
        if (!m_done) {
             std::cv_status ret = m_cv.wait_for(lk, std::chrono::seconds(m_timeout));
             if (ret == std::cv_status::timeout) {
                return std::make_pair(RpcCBStatus::RPC_TIMEOUT, std::string(m_resp_data));
             }
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
    int m_timeout;
    std::string m_resp_data;
    RpcCBStatus m_cb_st;
    
};
};
#endif
