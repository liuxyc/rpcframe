/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include "Queue.h"
#include <memory>
#include <string.h>

namespace rpcframe
{

#define RPCFRAME_VER 1

class RpcServerConnWorker;

enum class PkgIOStatus {
  FAIL = -1,
  PARTIAL = -2,
  TIME_OUT = -3,
  NODATA = -4,
  FULL = 0,
};

class request_pkg
{
public:
    request_pkg(uint32_t size, std::string conn_id)
    : data(nullptr)
    , data_len(size)
    , connection_id(conn_id)
    , conn_worker(nullptr)
    {
        data = new char[size];
    };
    ~request_pkg()
    {
        delete [] data;
    };
    char *data;
    uint32_t data_len;
    std::string connection_id;
    RpcServerConnWorker *conn_worker;
    std::chrono::system_clock::time_point gen_time;

    request_pkg(const request_pkg &) = delete;
    request_pkg &operator=(const request_pkg &) = delete;

};

class response_pkg
{
public:
    explicit response_pkg(uint32_t size)
    : data(nullptr)
    , data_len(size)
    {
        data = new char[size];
    };
    ~response_pkg()
    {
        delete [] data;
    };
    char *data;
    uint32_t data_len;
    std::chrono::system_clock::time_point gen_time;

    response_pkg(const response_pkg &) = delete;
    response_pkg &operator=(const response_pkg&) = delete;
};

typedef std::shared_ptr<request_pkg> ReqPkgPtr;
typedef std::shared_ptr<response_pkg> RespPkgPtr;
typedef Queue<ReqPkgPtr> ReqQueue;
typedef Queue<RespPkgPtr> RespQueue;
};
