/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_ISERVICE
#define RPCFRAME_ISERVICE

namespace rpcframe {

#define RPC_ADD_METHOD(class_name, method_name) m_method_map[#method_name] = &class_name::method_name;

#define RPC_METHOD_FUNC(method_name) static rpcframe::IService::ServiceRET method_name(const std::string &request_data, std::string &resp_data)

#define REG_METHOD(class_name) \
    typedef rpcframe::IService::ServiceRET (class_name::*METHOD_FUNC)(const std::string &, std::string &); \
    std::map<std::string, METHOD_FUNC> m_method_map; \
    ServiceRET runService(const std::string &method_name, const std::string &request_data, std::string &resp_data) { \
        if (m_method_map.find(method_name) != m_method_map.end()) { \
            METHOD_FUNC p_fun = m_method_map[method_name]; \
            return (this->*p_fun)(request_data, resp_data); \
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

    virtual ServiceRET runService(const std::string &method_name, const std::string &request_data, std::string &resp_data) = 0;

};
typedef std::unordered_map<std::string, IService *> ServiceMap;

};
#endif
