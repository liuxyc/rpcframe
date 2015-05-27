/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_RPCCLIENTCONN
#define RPCFRAME_RPCCLIENTCONN
#include <mutex>
#include <ctime>
#include <set>

#include "RpcPackage.h"
#include "Queue.h"

namespace rpcframe {

class server_resp_pkg
{
public:
    server_resp_pkg(uint32_t size)
    {
        data = new char[size];
        data_len = size;
    };
    ~server_resp_pkg()
    {
        delete [] data;
    };
    char *data;
    uint32_t data_len;

};

typedef std::pair<int, server_resp_pkg *> pkg_ret_t;

class RpcClientConn
{
public:
    RpcClientConn(int fd);
    ~RpcClientConn();

    pkg_ret_t getResponse();
    void reset();
    bool sendReq(const std::string &service_name, const std::string &method_name, const std::string &request_data, const std::string &reqid);

    int getFd();

    std::string m_seqid;

    std::string genRequestId();

private:
    bool readPkgLen(uint32_t &pkg_len);
    int readPkgData();

private:
    int m_fd;
    uint32_t m_cur_left_len;
    uint32_t m_cur_pkg_size;
    server_resp_pkg *m_rpk;
    bool is_connected;
    std::mutex m_mutex;
      

    
};


};

#endif
