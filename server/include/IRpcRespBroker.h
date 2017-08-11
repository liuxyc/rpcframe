/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include <string>
#include <memory>
#include <google/protobuf/message.h>

#include "RpcDefs.h"

namespace rpcframe {

/**
 * @brief IRpcRespBroker help server send the response in async
 */
class IRpcRespBroker
{
public:
    virtual ~IRpcRespBroker(){};
    /**
     * @brief send response data
     *
     * @param resp_data
     *
     * @return 
     */
    virtual bool response(RpcStatus rs) = 0;

    /**
     * @brief check whether need response
     *
     * @return 
     */
    virtual bool isNeedResp() = 0;

    /**
     * @brief check whether already responsed
     *
     * @return 
     */
    virtual bool isResponed() = 0;

    /**
     * @brief check whether the request is http request
     *
     * @return 
     */
    virtual bool isFromHttp() = 0;

    /**
     * @brief allocate a char * buffer for user, user need to fill response data into this buffer
     *
     * @param len request length of the char * buffer
     *
     * @return pointer of the char * buffer
     */
    virtual char *allocRespBuf(size_t len) = 0;
    virtual char *allocRespBufFrom(const std::string &resp) = 0;
    virtual char *allocRespBufFrom(const google::protobuf::Message &resp) = 0;

    virtual void setReturnVal(RpcStatus rs) = 0;
};
};
