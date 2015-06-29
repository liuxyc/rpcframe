# rpcframe [![Build Status](https://travis-ci.org/liuxyc/rpcframe.svg?branch=master)](https://travis-ci.org/liuxyc/rpcframe)
rpcframe is a simple rpc framework writen in C++11. It support Client side sync/async timeout call and Server side async response.<br>
The rpc interface is simple, it send and receive std::string as raw data.<br>
## Client side interface: <br>
RpcClient(rpcframe::RpcClientConfig &cfg, const std::string &service_name);<br>
RpcClient::call(const std::string &method_name, const std::string &request_data, std::string &response_data, uint32_t timeout)<br>
RpcClient::async_call(const std::string &method_name, const std::string &request_data, uint32_t timeout, RpcClientCallBack *cb_obj);<br>
## Server side implement rpcframe::IService and write member method:<br>
rpcframe::IService::ServiceRET method_name(const std::string &req_data, std::string &resp_data, rpcframe::RpcRespBroker *resp_broker);
<br>
#### Please check test/server_test.cpp, test/client_test.cpp for more usage details.<br>

dependence：<br>
    Linux kernel > 2.6.30<br>
    Gcc > 4.8.2<br>
    libuuid<br>
    SCons 2.3.1<br>
