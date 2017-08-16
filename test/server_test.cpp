/*
 * Copyright (c) 2015-2017, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include <chrono>
#include <random>
#include <memory>
#include <thread>
#include <mutex>

#include <signal.h>
#include <cstring>

#include "RpcServer.h"

class MyService_async: public rpcframe::IService
{
public:
    MyService_async(){
        //to make your method to be callable, must write RPC_ADD_METHOD(your_class_name, your_method_name)
        RPC_ADD_METHOD(MyService_async, test_method_async)
    };
    virtual ~MyService_async(){
        for(auto th : m_t) {
          th->join();
          delete th;
        }
    };

    rpcframe::RpcStatus test_method_async(const rpcframe::RawData &req_data, rpcframe::IRpcRespBrokerPtr resp_broker) 
    {
        printf("test_method_async get %s\n", req_data.data);
        //make a async response
        m_t.push_back(new std::thread([resp_broker](){
                //must delete broker after call resp_broker->response, we use std::unique_ptr do it for us
                //std::unique_ptr<rpcframe::IRpcRespBroker> broker_ptr(resp_broker);
                std::this_thread::sleep_for(std::chrono::seconds(5));
                printf("i'm async method\n");
                std::string ss("my feedback async");
                char *p_ss = resp_broker->allocRespBuf(ss.size() + 1);
                strcpy(p_ss, ss.c_str());
                resp_broker->response(rpcframe::RpcStatus::RPC_SERVER_OK);
                }));
        /*
           NOTICE:Don't delete resp_broker if you return rpcframe::RpcStatus::RPC_SERVER_OK, RcpServer will delete it for you.

           Return rpcframe::RpcStatus::RPC_SERVER_NONE, means you don't want RpcServer send response in mathod function immediatly
           ,you can hold resp_broker and send the response by resp_broker later or not.

           Not send response will cause client side timeout or hang on a "no timeout call", but this is 
           reasonable if client didn't set callback(ONE_WAY call)
           */
        return rpcframe::RpcStatus::RPC_SERVER_NONE;
    };
    std::vector<std::thread *> m_t;

};

class MyService: public rpcframe::IService
{
public:
    MyService(){
        //to make your method to be callable, must write RPC_ADD_METHOD(your_class_name, your_method_name)
        RPC_ADD_METHOD(MyService, test_method_5sec_delay)
        RPC_ADD_METHOD(MyService, test_method_random_delay)
        RPC_ADD_METHOD(MyService, test_method_fast_return)
        RPC_ADD_METHOD_NOHTTP(MyService, test_method_big_resp)
        RPC_ADD_METHOD(MyService, test_method_echo)
        m_cnt = 0;
    };
    virtual ~MyService(){};

    rpcframe::RpcStatus test_method_5sec_delay(const rpcframe::RawData &req_data, 
                                               rpcframe::IRpcRespBrokerPtr resp_broker) 
    {
        //printf("my method get %s\n", request_data.c_str());
      std::string resp_data = "my feedback";
      m_mutex.lock();
      printf("cnt: %d\n", m_cnt++);
      //printf("data: %s\n", request_data.c_str());
      m_mutex.unlock();
      //make timeout
      std::this_thread::sleep_for(std::chrono::seconds(5));
      if (resp_broker->isFromHttp()) {
        resp_data = "<html><body><h1>this response for http 5 secs delay</h1></body></html>";
      }
      resp_broker->allocRespBufFrom(resp_data);

      return rpcframe::RpcStatus::RPC_SERVER_OK;
    };

    rpcframe::RpcStatus test_method_random_delay(const rpcframe::RawData &req_data, 
                                                rpcframe::IRpcRespBrokerPtr resp_broker) 
    {
      std::string resp_data = "my feedback_random_delay";
      //generate 0-5 seconds delay
      std::random_device rd;
      uint32_t t = rd() % 5;
      std::this_thread::sleep_for(std::chrono::seconds(t));
      resp_broker->allocRespBufFrom(resp_data);
      return rpcframe::RpcStatus::RPC_SERVER_OK;
    };

    rpcframe::RpcStatus test_method_fast_return(const rpcframe::RawData &req_data, 
                                                rpcframe::IRpcRespBrokerPtr resp_broker) 
    {
      resp_broker->allocRespBufFrom("my feedback");
      return rpcframe::RpcStatus::RPC_SERVER_OK;
    };

    rpcframe::RpcStatus test_method_big_resp(const rpcframe::RawData &req_data, 
                                            rpcframe::IRpcRespBrokerPtr resp_broker) 
  {
      std::string resp_data(1024*1024*40, 'a');
      resp_broker->allocRespBufFrom(resp_data);
      return rpcframe::RpcStatus::RPC_SERVER_OK;
    };

    rpcframe::RpcStatus test_method_echo(const rpcframe::RawData &req_data, 
                                        rpcframe::IRpcRespBrokerPtr resp_broker) 
    {
      std::string resp_data(req_data.data, req_data.data_len);
      resp_broker->allocRespBufFrom(resp_data);
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
    int rpcport = 8801;
    int httpport = 8889;
    if( argc < 3) {
        printf("usage: %s [rpc port] [http port]\n", argv[0]);
    }
    else {
        rpcport = std::stoi(argv[1]);
        httpport = std::stoi(argv[2]);
    }

    auto endp = std::make_pair("127.0.0.1", rpcport);
    rpcframe::RpcServerConfig cfg(endp);
    cfg.setThreadNum(4);
    cfg.enableHttp(httpport, 4);
    //cfg.disableHttp();
    rpcframe::RpcServer rpcServer(cfg);
    MyService ms;
    std::unique_ptr<MyService_async> ms_async(new MyService_async());
    //bind service_name to service instance
    rpcServer.addService<MyService>("test_service", &ms);
    rpcServer.addService<MyService_async>("test_service_async", ms_async.get());

    //rpcframe will create instance per work thread
    //rpcServer.addService<MyService>("test_service", nullptr);
    //rpcServer.addService<MyService_async>("test_service_async", nullptr);
    
    g_server_handler = &rpcServer;
    rpcServer.start();
    printf("server_test stoped\n");
}
