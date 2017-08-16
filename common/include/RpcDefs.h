/*
 * Copyright (c) 2015-2017, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include <utility>
#include <string>
#include <functional>
#include <memory>

namespace rpcframe
{

    enum class RpcStatus {
        RPC_SEND_OK = 0,
        RPC_SEND_FAIL,
        RPC_REQ_TOO_LARGE,
        RPC_DISCONNECTED,
        RPC_SEND_TIMEOUT,
        RPC_CB_TIMEOUT = 5,
        RPC_SRV_NOTFOUND,
        RPC_METHOD_NOTFOUND,
        RPC_SERVER_OK,
        RPC_SERVER_NONE,
        RPC_SERVER_FAIL,
        RPC_MALLOC_PKG_FAIL,
    };

    class RawDataImpl;
    class RawData {
      public:
        RawData();
        explicit RawData(const std::string &s);
        RawData(const char *d, size_t l);
        explicit RawData(const RawData& r);
        RawData &operator=(const RawData& r);
        ~RawData();
        size_t size() const;
        char *data;
        size_t data_len;
        RawDataImpl *m_impl;
    };

    class IRpcRespBroker;

    typedef std::shared_ptr<IRpcRespBroker> IRpcRespBrokerPtr;
    typedef std::function<rpcframe::RpcStatus(const RawData &req, IRpcRespBrokerPtr)> RPC_FUNC_T;
    typedef std::pair<std::string, int> Endpoint;

    const size_t max_protobuf_data_len = 10 * 1024 * 1024;


    /*
     * Requestbinary format:
     * |--> uint32_t(the length of following data) <--|--> uint32_t(the length of protobuf data) <--|--> protobuf data + user data(if user data < 1MB)ref: common/proto/rpc.proto) <--|-->user data(if user data > 1MB)<--|
     */

    /*
     * Response binary format:
     * |--> uint32_t(the length of following data) <--|--> uint32_t(the length of protobuf data) <--|--> protobuf data, ref: common/proto/rpc.proto) <--|--> user data <--|--> return valuse <--| 
     */
};
