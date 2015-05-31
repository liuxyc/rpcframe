# rpcframe
rpcframe is a simple rpc framework writen in C++11. It support Client side sync/asyc call.<br>
The rpc interface is simple, it send and receive std::string as raw data.<br>
## Client side call: <br>
RpcClient::call(const std::string &method_name, const std::string &request_data, std::string &response_data, uint32_t timeout)<br>
RpcClient::async_call(const std::string &method_name, const std::string &request_data, uint32_t timeout, RpcClientCallBack *cb_obj);<br>
## Server side implement rpcframe::IServicer and write member method:<br>
rpcframe::IService::ServiceRET test_method1(const std::string &request_data, std::string &resp_data);
<br>
#### Please check test/server_test.cpp, test/client_test.cpp for more usage details.<br>

dependence：<br>
    linux kernel > 2.6.30<br>
    gcc > 4.8.2 , support c++11<br>
    libuuid<br>
    SCons 2.3.1<br>
