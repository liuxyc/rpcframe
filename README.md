# rpcframe
[![Build Status](https://travis-ci.org/liuxyc/rpcframe.svg?branch=master)](https://travis-ci.org/liuxyc/rpcframe)
[![Cov Status](https://scan.coverity.com/projects/7717/badge.svg)](https://scan.coverity.com/projects/liuxyc-rpcframe)

rpcframe is a simple rpc framework writen in C++11. It support Client side sync/async timeout call and Server side async response.
The rpc interface is simple, it send and receive std::string as raw data.
## Client side interface:
```
RpcClient(rpcframe::RpcClientConfig &cfg, const std::string &service_name);
RpcClient::call(const std::string &method_name, const std::string &request_data, std::string &response_data, uint32_t timeout)
RpcClient::async_call(const std::string &method_name, const std::string &request_data, uint32_t timeout, std::shared_ptr<RpcClientCallBack> cb_obj);
```
## Server side implement rpcframe::IService and write member method:
```
rpcframe::RpcStatus method_name(const std::string &req_data, std::string &resp_data, rpcframe::RpcRespBrokerPtr resp_broker);
```
##HTTP interface:
```
HTTP GET http://127.0.0.1:8000/[service_name]/[method_name]
curl --data "hello server"  http://127.0.0.1:8000/test_service/test_method
```
#### Please check test/server_test.cpp, test/client_test.cpp for more usage details.

Dependence：

     Linux kernel > 2.6.30
     Gcc > 4.8.2
     libuuid
     protobuf-2.6.1
     gtest(for unittest)
     SCons 2.3.1
