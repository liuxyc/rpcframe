# rpcframe
rpcframe is a simple rpc framework writen in C++11. It support Client side sync/asyc call.<br>
The rpc interface is simple:<br>
client: call/async_call (const std::string &method_name, const std::string &request_data, std::string &response_data, int timeout)<br>
server: runService(const std::string &method_name, const std::string &request_data, std::string &resp_data)<br>
<br>
Please check test/server_test.cpp, test/client_test.cpp for more details.<br>

dependence：<br>
    linux kernel > 2.6.30<br>
    gcc > 4.8.2 , support c++11<br>
    libuuid<br>
    SCons 2.3.1<br>
