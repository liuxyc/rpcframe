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

#include "RpcDefs.h"

namespace rpcframe
{

class RpcEventLooper;

class RpcClientCallBack 
{
public:
    RpcClientCallBack() 
    : m_timeout(0)
    , m_reqid("")
    , m_has_timeout(false)
    {
    };
    virtual ~RpcClientCallBack(){};

    /**
     * @brief implement this method for your callback
     *
     * @param status
     * @param response_data
     */
    virtual void callback(const RpcStatus status, const std::string &response_data) = 0;

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

protected:
    std::string m_type_mark;
    uint32_t m_timeout;
    std::string m_reqid;
    bool m_has_timeout;
    
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
     * @param cb_obj
     *
     * @return 
     */
    RpcStatus async_call(const std::string &method_name, const std::string &request_data, uint32_t timeout, RpcClientCallBack *cb_obj);

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
