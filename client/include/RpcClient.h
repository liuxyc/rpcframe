/*
 * Copyright (c) 2015-2017, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <google/protobuf/message.h>

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
    RpcClientCallBack();
    virtual ~RpcClientCallBack();

    /**
     * @brief implement this method for your callback
     *        NOTICE:callback may called in multithread
     *
     * @param status
     * @param response_data
     */
    virtual void callback(const RpcStatus status, const RawData &) = 0;

    void callback_safe(const RpcStatus status, const RawData &resp_data);

    std::string getType();

    void setType(const std::string &type);
    void setTimeout(uint32_t timeout);
    uint32_t getTimeout();
    void setReqId(const std::string &reqid);
    std::string getReqId();
    void markTimeout();
    bool isTimeout();
    /**
     * @brief set a CallbackObject as shared will have "timeout/success come both" issue
     *        because we can't identify the callback source
     *
     * @param isshared bool, default is false
     */
    void setShared(bool isshared);

protected:
    std::string m_type_mark;
    uint32_t m_timeout;
    std::string m_reqid;
    bool m_has_timeout;
    bool m_is_done;
    bool m_is_shared;

private:
    std::mutex m_mutex;
    
};

class RpcClientConfig
{
public:
    explicit RpcClientConfig(const std::vector<Endpoint> &eps);
    ~RpcClientConfig();
    void setThreadNum(uint32_t thread_num);
    void setMaxReqPkgSize(uint32_t max_req_size);
    uint32_t getThreadNum();

    uint32_t m_thread_num;
    std::string m_hostname;
    int m_port;
    int m_connect_timeout;
    uint32_t m_max_req_size;
    std::vector<Endpoint> m_eps;
};

//NOTICE: start/stop RpcClient is heavy, keep the instance as long as possiable
class RpcClient
{
public:
    RpcClient(rpcframe::RpcClientConfig &cfg, const std::string &service_name);
    ~RpcClient();

    /**
     * @brief sync call
     *
     * @param method_name
     * @param request_data
     * @param response_data
     * @param timeout
     *
     * @return 
     */
    RpcStatus call(const std::string &method_name, const RawData &request_data, RawData &, uint32_t timeout);
    RpcStatus call(const std::string &method_name, const google::protobuf::Message &request_data, RawData &, uint32_t timeout);

    /**
     * @brief async call
     *
     * @param method_name
     * @param request_data
     * @param timeout
     * @param cb_obj RpcClient will not delete cb_obj, user should delete if it no longer used
     *
     * @return 
     */
    RpcStatus async_call(const std::string &method_name, const RawData &request_data, uint32_t timeout, std::shared_ptr<RpcClientCallBack> cb_obj);
    RpcStatus async_call(const std::string &method_name, const google::protobuf::Message &request_data, uint32_t timeout, std::shared_ptr<RpcClientCallBack> cb_obj);

    const RpcClientConfig &getConfig();
    void reloadEndpoints(const std::vector<Endpoint> &eps);

    void waitAllCBDone(uint32_t timeout);

    RpcClient(const RpcClient &) = delete;
    RpcClient &operator=(const RpcClient &) = delete;

private:
    RpcClientConfig m_cfg;
    std::string m_servicename;
    std::unique_ptr<RpcEventLooper> m_ev;
    std::mutex m_mutex;

    std::vector<std::unique_ptr<std::thread>> m_thread_vec;
};

};
