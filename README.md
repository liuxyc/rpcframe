# rpcframe
rpcframe is a simple rpc framework writen in C++11. It support Client side sync/asyc call.
The rpc interface is simple:
client: call/async_call (const std::string &method_name, const std::string &request_data, std::string &response_data, int timeout)
server: runService(const std::string &method_name, const std::string &request_data, std::string &resp_data)

check test/server_test.cpp, test/client_test.cpp for more details.

dependence：
      gcc > 4.8.2 , support c++11
      libuuid
      linux kernel > 2.6.30
      SCons 2.3.1
