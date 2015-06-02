/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include "RpcServer.h"
#include "IService.h"
#include <chrono>
#include <random>
#include <memory>

class MyService_async: public rpcframe::IService
{
public:
    //to make your method to be callable, must write REG_METHOD(your_class_name) here one time
    REG_METHOD(MyService_async)

    MyService_async(){
        //to make your method to be callable, must write RPC_ADD_METHOD(your_class_name, your_method_name)
        RPC_ADD_METHOD(MyService_async, test_method_async)
    };
    virtual ~MyService_async(){};

    //method1
    rpcframe::IService::ServiceRET test_method_async(const std::string &request_data, std::string &resp_data, rpcframe::RpcRespBroker *resp_broker) {
        //printf("my method get %s\n", request_data.c_str());
        //make a async response
        std::thread *t = new std::thread([resp_broker](){
                //must delete broker after call resp_broker->response
                std::unique_ptr<rpcframe::RpcRespBroker> broker_ptr(resp_broker);
                std::this_thread::sleep_for(std::chrono::seconds(5));
                broker_ptr->response("my feedback async");
                });
        /*
           Don't delete resp_broker if you return rpcframe::IService::ServiceRET::S_OK

           Return rpcframe::IService::ServiceRET::S_NONE, means you don't want RpcServer send response
           ,you can send the response by resp_broker later or not send never(just delete resp_broker)

           Not sending response will cause client side timeout or halt on "no timeout call", but this is 
           reasonable if client didn't set callback(ONE_WAY call)
           */
        return rpcframe::IService::ServiceRET::S_NONE;
    };

};

class MyService: public rpcframe::IService
{
public:
    //to make your method to be callable, must write REG_METHOD(your_class_name) here one time
    REG_METHOD(MyService)

    MyService(){
        //to make your method to be callable, must write RPC_ADD_METHOD(your_class_name, your_method_name)
        RPC_ADD_METHOD(MyService, test_method)
        RPC_ADD_METHOD(MyService, test_method1)
        RPC_ADD_METHOD(MyService, test_method2)
        m_cnt = 0;
    };
    virtual ~MyService(){};

    //method1
    rpcframe::IService::ServiceRET test_method(const std::string &request_data, std::string &resp_data, rpcframe::RpcRespBroker *resp_broker) {
        //printf("my method get %s\n", request_data.c_str());
        resp_data = "my feedback";
        m_mutex.lock();
        printf("cnt: %d\n", m_cnt++);
        m_mutex.unlock();
        //make timeout
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return rpcframe::IService::ServiceRET::S_OK;
    };

    //method2
    rpcframe::IService::ServiceRET test_method1(const std::string &request_data, std::string &resp_data, rpcframe::RpcRespBroker *resp_broker) {
        //printf("my method1 get %s\n", request_data.c_str());
        resp_data = "my feedback1";
        //generate 0-5 seconds delay
        std::random_device rd;
        uint32_t t = rd() % 5;
        std::this_thread::sleep_for(std::chrono::seconds(t));
        return rpcframe::IService::ServiceRET::S_OK;
    };

    //method3
    rpcframe::IService::ServiceRET test_method2(const std::string &request_data, std::string &resp_data, rpcframe::RpcRespBroker *resp_broker) {
        //printf("my method1 get %s\n", request_data.c_str());
        resp_data = "my feedback2";
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
    MyService_async ms_async;
    //bind service_name to service instants
    rpcServer.addService("test_service", &ms);
    rpcServer.addService("test_service_async", &ms_async);
    rpcServer.start();
    rpcServer.stop();
}
