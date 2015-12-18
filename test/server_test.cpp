/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include "RpcServer.h"
#include "IService.h"
#include <chrono>
#include <random>
#include <memory>
#include <thread>
#include <mutex>

#include <signal.h>
#include <cstring>

class MyService_async: public rpcframe::IService
{
public:
    //to make your method to be callable, must write REG_METHOD(your_class_name) here one time
    REG_METHOD(MyService_async)

    MyService_async(){
        //to make your method to be callable, must write RPC_ADD_METHOD(your_class_name, your_method_name)
        RPC_ADD_METHOD(MyService_async, test_method_async)
        m_t = nullptr;
    };
    virtual ~MyService_async(){
        if(m_t != nullptr) {
          m_t->join();
          delete m_t;
        }
    };

    //method1
    rpcframe::RpcStatus test_method_async(const std::string &request_data, 
                                                     std::string &resp_data, 
                                                     rpcframe::IRpcRespBroker *resp_broker) 
    {
        printf("test_method_async get %s\n", request_data.c_str());
        //make a async response
        m_t = new std::thread([resp_broker](){
                //must delete broker after call resp_broker->response, we use std::unique_ptr do it for us
                std::unique_ptr<rpcframe::IRpcRespBroker> broker_ptr(resp_broker);
                std::this_thread::sleep_for(std::chrono::seconds(5));
                broker_ptr->response("my feedback async");
                });
        m_t->get_id();
        /*
           NOTICE:Don't delete resp_broker if you return rpcframe::RpcStatus::RPC_SERVER_OK, RcpServer will delete it for you.

           Return rpcframe::RpcStatus::RPC_SERVER_NONE, means you don't want RpcServer send response in mathod function immediatly
           ,you can send the response by resp_broker later or not send ever(just delete resp_broker)

           Not send response will cause client side timeout or hang on a "no timeout call", but this is 
           reasonable if client didn't set callback(ONE_WAY call)
           */
        return rpcframe::RpcStatus::RPC_SERVER_NONE;
    };
    std::thread *m_t;

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
        RPC_ADD_METHOD(MyService, test_method_big_resp)
        m_cnt = 0;
    };
    virtual ~MyService(){};

    //method1
    rpcframe::RpcStatus test_method(const std::string &request_data, 
                                               std::string &resp_data, 
                                               rpcframe::IRpcRespBroker *resp_broker) 
    {
        //printf("my method get %s\n", request_data.c_str());
        resp_data = "my feedback";
        m_mutex.lock();
        printf("cnt: %d\n", m_cnt++);
        //printf("data: %s\n", request_data.c_str());
        m_mutex.unlock();
        //make timeout
        std::this_thread::sleep_for(std::chrono::seconds(5));
        if (resp_broker->isFromHttp()) {
            resp_data = "<html><body><h1>this response for http</h1></body></html>";
        }

        return rpcframe::RpcStatus::RPC_SERVER_OK;
    };

    //method2
    rpcframe::RpcStatus test_method1(const std::string &request_data, 
                                                std::string &resp_data, 
                                                rpcframe::IRpcRespBroker *resp_broker) 
    {
        //printf("my method1 get %s\n", request_data.c_str());
        resp_data = "my feedback1";
        //generate 0-5 seconds delay
        std::random_device rd;
        uint32_t t = rd() % 5;
        std::this_thread::sleep_for(std::chrono::seconds(t));
        return rpcframe::RpcStatus::RPC_SERVER_OK;
    };

    //method3
    rpcframe::RpcStatus test_method2(const std::string &request_data, 
                                                std::string &resp_data, 
                                                rpcframe::IRpcRespBroker *resp_broker) 
    {
        //printf("my method1 get %s\n", request_data.c_str());
        resp_data = std::string("my feedback3");
        return rpcframe::RpcStatus::RPC_SERVER_OK;
    };

    //method big resp
    rpcframe::RpcStatus test_method_big_resp(const std::string &request_data, 
                                                        std::string &resp_data, 
                                                        rpcframe::IRpcRespBroker *resp_broker) 
    {
        //printf("my method1 get %s\n", request_data.c_str());
        resp_data = std::string(1024*1024*40, 'a');
        return rpcframe::RpcStatus::RPC_SERVER_OK;
    };

    int m_cnt;
    std::mutex m_mutex;

};

rpcframe::RpcServer *g_server_handler = nullptr;

void quit_handler(int signum) 
{
  if (g_server_handler != nullptr) {
    g_server_handler->stop();
  }
}

int main(int argc, char * argv[])
{
    struct sigaction sigint;
    memset(&sigint, 0, sizeof(struct sigaction));
    sigint.sa_handler = quit_handler;
    if (sigaction(SIGINT, &sigint, NULL) == -1) {
      perror("set signal handler fail\n");
    }

    auto endp = std::make_pair("127.0.0.1", 8801);
    rpcframe::RpcServerConfig cfg(endp);
    cfg.setThreadNum(8);
    cfg.enableHttp(8000, 4);
    //cfg.disableHttp();
    rpcframe::RpcServer rpcServer(cfg);
    MyService ms;
    MyService_async *ms_async = new MyService_async();
    //bind service_name to service instance
    rpcServer.addService("test_service", &ms);
    rpcServer.addService("test_service_async", ms_async);
    g_server_handler = &rpcServer;
    rpcServer.start();
    delete ms_async;
    printf("server_test stoped\n");
}
