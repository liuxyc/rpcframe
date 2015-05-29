/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include "RpcServer.h"
#include "IService.h"
#include <chrono>
#include <random>

class MyService: public rpcframe::IService
{
public:
    //to make your method to be callable, must write REG_METHOD(your_class_name) here one time
    REG_METHOD(MyService)

    MyService(){
        //to make your method to be callable, must write RPC_ADD_METHOD(your_class_name, your_method_name)
        RPC_ADD_METHOD(MyService, test_method)
        RPC_ADD_METHOD(MyService, test_method1)
        m_cnt = 0;
    };
    virtual ~MyService(){};

    //method1
    rpcframe::IService::ServiceRET test_method(const std::string &request_data, std::string &resp_data) {
        //printf("my method get %s\n", request_data.c_str());
        resp_data = "my feedback";
        m_mutex.lock();
        printf("cnt: %d\n", m_cnt++);
        m_mutex.unlock();
        return rpcframe::IService::ServiceRET::S_OK;
    };

    //method2
    rpcframe::IService::ServiceRET test_method1(const std::string &request_data, std::string &resp_data) {
        //printf("my method1 get %s\n", request_data.c_str());
        resp_data = "my feedback1";
        //generate 0-5 seconds delay
        std::random_device rd;
        uint32_t t = rd() % 5;
        std::this_thread::sleep_for(std::chrono::seconds(t));
        return rpcframe::IService::ServiceRET::S_OK;
    };

    int m_cnt;
    std::mutex m_mutex;

};

int main(int argc, char * argv[])
{
    auto endp = std::make_pair("127.0.0.1", 8801);
    rpcframe::RpcServerConfig cfg(endp);
    cfg.setThreadNum(4);
    rpcframe::RpcServer rpcServer(cfg);
    MyService ms;
    //bind service_name to service instants
    rpcServer.addService("test_service", &ms);
    rpcServer.start();
    rpcServer.stop();
}
