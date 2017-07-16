/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */
#include "RpcServerHttpConn.h"

#include <errno.h>
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <fcntl.h>  
#include <string.h>
#include <unistd.h>

#include <chrono>

#include "util.h"
#include "RpcServerImpl.h"
#include "RpcServerConfig.h"

namespace rpcframe
{

//----http request parser callback-----
int request_url_cb (http_parser *p, const char *buf, size_t len)
{
    RpcServerHttpConn *httpconn = (RpcServerHttpConn *)p->data;
    httpconn->appendURL(buf, len);
    return 0;
}

int headers_complete_cb (http_parser *p)
{
    RpcServerHttpConn *httpconn = (RpcServerHttpConn *)p->data;
    if(p->method == HTTP_GET) {
        httpconn->bodyFinal(true);
    }
    if(p->method == HTTP_POST && p->content_length == 0) {
        RPC_LOG(RPC_LOG_LEV::WARNING, "not support content_length 0");
        httpconn->setFail(true);
    }
    httpconn->setKeepAlive(http_should_keep_alive(p));
    return 0;
}

int body_cb (http_parser *p, const char *buf, size_t len)
{
    RpcServerHttpConn *httpconn = (RpcServerHttpConn *)p->data;
    httpconn->appendBody(buf, len);
    httpconn->bodyFinal(http_body_is_final(p));
    return 0;
}
//-----

RpcServerHttpConn::RpcServerHttpConn(int fd, const char *id, RpcServerImpl *server)
: RpcServerConn(fd, id, server)
{
    m_is_body_final = false;
    memset(&m_setting, 0, sizeof(m_setting));
    m_setting.on_body = body_cb;
    m_setting.on_url = request_url_cb;
    m_setting.on_headers_complete = headers_complete_cb;

    m_http_parser = (http_parser *)malloc(sizeof(http_parser));
    http_parser_init(m_http_parser, HTTP_REQUEST);
    m_http_parser->data = this;
}

RpcServerHttpConn::~RpcServerHttpConn()
{
    free(m_http_parser);
}

void RpcServerHttpConn::appendBody(const char *buf, int len)
{
    m_body.append(buf, len);
}

void RpcServerHttpConn::appendURL(const char *buf, int len)
{
    m_url.append(buf, len);
}

void RpcServerHttpConn::setFail(bool f)
{
    m_parse_fail = f;
}

void RpcServerHttpConn::setKeepAlive(bool f)
{
    m_keep_alive = f;
}

pkg_ret_t RpcServerHttpConn::getRequest() 
{
    if (!is_connected) {
        RPC_LOG(RPC_LOG_LEV::WARNING, "connection already disconnected");
        return pkg_ret_t(-1, nullptr);
    }
    while(1) {
        char data[4096];
        errno = 0;
        int rsize = recv(m_fd, data, sizeof(data), 0);  
        if (rsize < 0) {
            if( errno == EAGAIN || errno == EINTR) {
                //ET trigger case
                //will continue until we read the full package length
                RPC_LOG(RPC_LOG_LEV::DEBUG, "socket EAGAIN");
                return pkg_ret_t(0, nullptr);
            }
            else {
                RPC_LOG(RPC_LOG_LEV::ERROR, "try recv pkg len error %s, need recv %d:%d", strerror(errno), rsize, sizeof(data));
                return pkg_ret_t(-1, nullptr);
            }
        }
        else {
            printf("http %s\n", data);
            int nparsed = http_parser_execute(m_http_parser, &m_setting, data, rsize);

            if (m_http_parser->upgrade) {
                /* handle new protocol */
                RPC_LOG(RPC_LOG_LEV::INFO, "can't handle upgrade");
                return pkg_ret_t(-1, nullptr);
            } else if (nparsed != rsize) {
                /* Handle error. Usually just close the connection. */
                RPC_LOG(RPC_LOG_LEV::INFO, "parser Peer disconected");
                return pkg_ret_t(-1, nullptr);
            }
            if (rsize == 0) {
                RPC_LOG(RPC_LOG_LEV::INFO, "Peer disconected");
                return pkg_ret_t(-1, nullptr);
            }
            if(m_parse_fail) {
                RPC_LOG(RPC_LOG_LEV::WARNING, "parse fail");
                return pkg_ret_t(-1, nullptr);
            }
            if(m_is_body_final) {
                m_is_body_final = false;
                RPC_LOG(RPC_LOG_LEV::WARNING, "get compelete http req, url:%s", m_url.c_str());
                std::string::size_type service_pos = m_url.find('/', 1);
                std::string service_name = m_url.substr(1, service_pos - 1);
                std::string method_name = m_url.substr(service_pos + 1, m_url.size());
                RpcInnerReq req;
                req.set_version(1);
                req.set_type(RpcInnerReq::TWO_WAY);
                req.set_request_id("http");
                req.set_service_name(service_name);
                req.set_method_name(method_name);
                req.set_data(m_body);
                req.set_timeout(0);
                uint32_t proto_len = req.ByteSize();
                uint32_t proto_hdr = htonl(proto_len);
                auto reqpkg = std::make_shared<request_pkg>(proto_len + sizeof(proto_len), m_seqid);
                reqpkg->is_from_http = true;
                memcpy(reqpkg->data, &proto_hdr, sizeof(proto_hdr));
                if (!req.SerializeToArray(reqpkg->data + sizeof(proto_hdr), proto_len)) {
                    RPC_LOG(RPC_LOG_LEV::ERROR, "Serialize req data fail");
                    return pkg_ret_t(-1, nullptr); 
                }
                reqpkg->data_len = proto_len;

                return pkg_ret_t(0, reqpkg);
            }
            else {
                return pkg_ret_t(0, nullptr);
            }
        }
    }
    return pkg_ret_t(-1, nullptr);
}


};
