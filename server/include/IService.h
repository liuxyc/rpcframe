/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_ISERVICE
#define RPCFRAME_ISERVICE
#include <map>
#include <string>

#include "IRpcRespBroker.h"
#include "RpcDefs.h"

namespace rpcframe {

#define RPC_ADD_METHOD(class_name, method_name) m_method_map[#method_name] = &class_name::method_name;

#define REG_METHOD(class_name) \
    typedef rpcframe::RpcStatus (class_name::*METHOD_FUNC)(const std::string &, std::string &, rpcframe::IRpcRespBroker *); \
    std::map<std::string, METHOD_FUNC> m_method_map; \
    rpcframe::RpcStatus runService(const std::string &method_name, \
                          const std::string &request_data, \
                          std::string &resp_data, \
                          rpcframe::IRpcRespBroker *resp_broker) \
    { \
        if (m_method_map.find(method_name) != m_method_map.end()) { \
            METHOD_FUNC p_fun = m_method_map[method_name]; \
            return (this->*p_fun)(request_data, resp_data, resp_broker); \
        }  \
        else { \
            return rpcframe::RpcStatus::RPC_METHOD_NOTFOUND;  \
        }  \
    };

class IService
{
public:
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
    virtual RpcStatus runService(const std::string &method_name, const std::string &req_data, std::string &resp_data, IRpcRespBroker *resp_broker) = 0;

};

};
#endif
