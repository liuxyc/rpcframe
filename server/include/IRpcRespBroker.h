/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_IRPCRESPBROKER
#define RPCFRAME_IRPCRESPBROKER
#include <string>
#include <memory>

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
    virtual bool response(const std::string &resp_data) = 0;

    /**
     * @brief check whether need response
     *
     * @return 
     */
    virtual bool isNeedResp() = 0;

    /**
     * @brief check whether the request is http request
     *
     * @return 
     */
    virtual bool isFromHttp() = 0;
};


};
#endif
