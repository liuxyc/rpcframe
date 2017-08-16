/*
 * Copyright (c) 2015-2017, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include <mutex>
#include <memory>
#include <string>
#include <map>

#include "RpcServerConn.h"
#include "http_parser.h"

namespace rpcframe {

typedef std::pair<int, std::shared_ptr<request_pkg> > pkg_ret_t;

class RpcServerImpl;
class EpollStruct;

class RpcServerHttpConn : public RpcServerConn
{
public:
    RpcServerHttpConn(int fd, const char *seqid, RpcServerImpl *server);
    ~RpcServerHttpConn();

    virtual pkg_ret_t getRequest() override;

    void appendBody(const char *buf, int len);
    void appendURL(const char *buf, int len);
    void setFail(bool f);
    void setKeepAlive(bool f);

    void bodyFinal(bool f) {m_is_body_final = f;};

private:
    http_parser *m_http_parser;
    http_parser_settings m_setting;
    std::map<std::string, std::string> m_headers;
    std::string m_body;
    std::string m_url;
    bool m_is_body_final = false;
    bool m_keep_alive = false;
    bool m_parse_fail = false;
};
};
