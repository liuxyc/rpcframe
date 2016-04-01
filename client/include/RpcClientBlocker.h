/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCCLIENTBLOCKER
#define RPCFRAME_RPCCLIENTBLOCKER
#include <vector>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "RpcClient.h"
#include "Queue.h"
#include "RpcPackage.h"
#include "util.h"

namespace rpcframe
{

class RpcClientBlocker: public RpcClientCallBack 
{
public:
    explicit RpcClientBlocker(int timeout)
    : RpcClientCallBack()
    , m_done(false)
    { 
        m_type_mark = "blocker"; 
        setTimeout(timeout);
    };
    virtual ~RpcClientBlocker() {};

    std::pair<RpcStatus, std::string> wait(const std::string &req_id) {
        std::unique_lock<std::mutex> lk(m_blocker_mutex);
        //NOTICE: don't touch m_resp_data here, because m_resp_data may already fulfilled
        if (!m_done) {
             std::cv_status ret = m_cv.wait_for(lk, std::chrono::seconds(getTimeout()));
             if (ret == std::cv_status::timeout) {
                RPC_LOG(RPC_LOG_LEV::DEBUG, "%s sync call timeout", req_id.c_str());
                return std::make_pair(RpcStatus::RPC_CB_TIMEOUT, m_resp_data);
             }
        }
        return std::make_pair(m_cb_st, m_resp_data);
    }

    virtual void callback(const RpcStatus status, const std::string &response_data) {
        std::unique_lock<std::mutex> lk(m_blocker_mutex);
        m_resp_data = response_data;
        m_cb_st = status;
        m_done = true;
        m_cv.notify_all();
    }

    std::string getRespData() {
        return m_resp_data;
    }

    RpcStatus getCBstatus() {
        return m_cb_st;
    }

private:
    std::mutex m_blocker_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_done;
    std::string m_resp_data;
    RpcStatus m_cb_st;
    
};
};
#endif
