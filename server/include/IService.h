/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_ISERVICE
#define RPCFRAME_ISERVICE

#include "RpcRespBroker.h"

namespace rpcframe {

//class rpcframe::RpcRespBroker;

#define RPC_ADD_METHOD(class_name, method_name) m_method_map[#method_name] = &class_name::method_name;

#define REG_METHOD(class_name) \
    typedef rpcframe::IService::ServiceRET (class_name::*METHOD_FUNC)(const std::string &, std::string &, rpcframe::RpcRespBroker *); \
    std::map<std::string, METHOD_FUNC> m_method_map; \
    ServiceRET runService(const std::string &method_name, const std::string &request_data, std::string &resp_data, rpcframe::RpcRespBroker *resp_broker) { \
        if (m_method_map.find(method_name) != m_method_map.end()) { \
            METHOD_FUNC p_fun = m_method_map[method_name]; \
            return (this->*p_fun)(request_data, resp_data, resp_broker); \
        }  \
        else { \
            return ServiceRET::S_FAIL;  \
        }  \
    };

class IService
{
public:
    enum class ServiceRET {
        S_OK,
        S_NONE,
        S_FAIL,
    };
    IService() {};
    virtual ~IService() {};

    
    /**
     * @brief 
     *
     * @param method_name
     * @param request_data
     * @param resp_data
     * @param resp_broker if you return S_NONE, you need to delete resp_broker after call resp_broker->pushResp() yourself.
     *
     * @return 
     */
    virtual ServiceRET runService(const std::string &method_name, const std::string &request_data, std::string &resp_data, RpcRespBroker *resp_broker) = 0;

};
typedef std::unordered_map<std::string, IService *> ServiceMap;

};
#endif
