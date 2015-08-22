/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCCLIENT
#define RPCFRAME_RPCCLIENT
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>

#include "RpcDefs.h"

namespace rpcframe
{

class RpcEventLooper;

class RpcClientCallBack 
{
public:
    /**
     * @brief Callback base class
     *        we suggest each async_call have it's own CallBack instance
     */
    RpcClientCallBack() 
    : m_timeout(0)
    , m_reqid("")
    , m_has_timeout(false)
    , m_is_done(false)
    , m_is_shared(false)
    {
    };
    virtual ~RpcClientCallBack(){};

    /**
     * @brief implement this method for your callback
     *        NOTICE:callback may called in multithread
     *
     * @param status
     * @param response_data
     */
    virtual void callback(const RpcStatus status, const std::string &response_data) = 0;

    void callback_safe(const RpcStatus status, const std::string &response_data) {
        //if a callback instance shared by many call, not use internal "m_is_done"
        if (m_is_shared) {
            callback(status, response_data);
        } 
        else {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_is_done) {
                callback(status, response_data);
                m_is_done = true;
            }
        }

    }

    std::string getType() {
        return m_type_mark;
    }

    void setType(const std::string &type) {
        m_type_mark = type;
    }
    void setTimeout(uint32_t timeout) {
        m_timeout = timeout;
    }
    uint32_t getTimeout() {
        return m_timeout;
    }
    void setReqId(const std::string &reqid) {
        m_reqid = reqid;
    }
    std::string getReqId() {
        return m_reqid;
    }
    void markTimeout() {
        m_has_timeout = true;
    }
    bool isTimeout() {
        return m_has_timeout;
    }
    /**
     * @brief set a CallbackObject as shared will have "timeout/success come both" issue
     *        because we can't identify the callback source
     *
     * @param isshared bool, default is false
     */
    void setShared(bool isshared) {
        m_is_shared = isshared;
    }

protected:
    std::string m_type_mark;
    uint32_t m_timeout;
    std::string m_reqid;
    bool m_has_timeout;
    bool m_is_done;
    bool m_is_shared;
    std::mutex m_mutex;
    
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
    int m_connect_timeout;
};

//NOTICE: start/stop RpcClient is heavy, keep the instance as long as possiable
class RpcClient
{
public:
    RpcClient(rpcframe::RpcClientConfig &cfg, const std::string &service_name);
    ~RpcClient();

    /**
     * @brief sync call, not thread safe
     *
     * @param method_name
     * @param request_data
     * @param response_data
     * @param timeout
     *
     * @return 
     */
    RpcStatus call(const std::string &method_name, const std::string &request_data, std::string &response_data, uint32_t timeout);

    /**
     * @brief async call, not thread safe
     *
     * @param method_name
     * @param request_data
     * @param timeout
     * @param cb_obj RpcClient will not delete cb_obj, user should delete if it no longer used
     *
     * @return 
     */
    RpcStatus async_call(const std::string &method_name, const std::string &request_data, uint32_t timeout, std::shared_ptr<RpcClientCallBack> cb_obj);

    const RpcClientConfig &getConfig();

    RpcClient(const RpcClient &) = delete;
    RpcClient &operator=(const RpcClient &) = delete;

private:
    RpcClientConfig m_cfg;
    bool m_isConnected;
    int m_fd;
    std::string m_servicename;
    RpcEventLooper *m_ev;
    std::mutex m_mutex;

    std::vector<std::thread *> m_thread_vec;
};

};
#endif
