/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCPACKAGE
#define RPCFRAME_RPCPACKAGE

#include "Queue.h"
#include <memory>

namespace rpcframe
{

enum class PkgIOStatus {
  FAIL = -1,
  PARTIAL = -2,
  TIME_OUT = -3,
  FULL = 0,
};

class request_pkg
{
public:
    request_pkg(uint32_t size, std::string conn_id)
    : connection_id(conn_id)
    , data(nullptr)
    , data_len(size)
    , http_conn(nullptr)
    {
        data = new char[size];
    };
    ~request_pkg()
    {
        delete [] data;
    };
    std::string connection_id;
    char *data;
    uint32_t data_len;
    void *http_conn;
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

typedef Queue<std::shared_ptr<request_pkg> > ReqQueue;
typedef Queue<std::shared_ptr<response_pkg> > RespQueue;
typedef std::shared_ptr<request_pkg> ReqPkgPtr;
typedef std::shared_ptr<response_pkg> RespPkgPtr;
};

#endif
