/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCPACKAGE
#define RPCFRAME_RPCPACKAGE
#include <utility>
#include <map>
#include <vector>

#include "Queue.h"

namespace rpcframe
{

class RpcConnection;

class request_pkg
{
public:
    request_pkg(uint32_t size, std::string conn_id)
    : data(NULL)
    , data_len(size)
    {
        connection_id = conn_id;
        data = new char[size];
    };
    ~request_pkg()
    {
        delete [] data;
    };
    std::string connection_id;
    char *data;
    uint32_t data_len;

};

class response_pkg
{
public:
    response_pkg()
    : data(NULL)
    , data_len(0)
    {
    };
    ~response_pkg()
    {
        delete data;
    };
    std::string *data;
    uint32_t data_len;


};

typedef Queue<rpcframe::request_pkg *> ReqQueue;
typedef Queue<rpcframe::response_pkg *> RespQueue;
};

#endif
