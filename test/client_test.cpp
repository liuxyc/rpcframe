/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <random>
#include "RpcClient.h"

class my_CB : public rpcframe::RpcClientCallBack 
{
public:
    my_CB()
    : rpcframe::RpcClientCallBack()
    {};
    virtual ~my_CB() {};

    virtual void callback(const rpcframe::RpcStatus status, const std::string &response_data) {
        if (status != rpcframe::RpcStatus::RPC_CB_OK) {
            printf("client cb return %d\n", status);
        }
        else {
            //printf("client cb return %d, got %s\n", status, response_data.c_str());
            printf("client cb return %d, got data len %d\n", status, response_data.length());
        }

    }
    
};

int main()
{
    int conn_cnt = 10;
    int pkg_cnt = 10000;
    auto endp = std::make_pair("127.0.0.1", 8801);
    rpcframe::RpcClientConfig ccfg(endp);

    rpcframe::RpcClient client(ccfg, "test_service");
    //fast async/sync call
    for (int cnt = 0; cnt < conn_cnt; ++cnt) {
        for (int pcnt = 0; pcnt < pkg_cnt; ++pcnt) {
            if(rpcframe::RpcStatus::RPC_SEND_OK != client.async_call("test_method2", std::string(1024 * 10, '*'), 10, new my_CB())) {
                printf("send fail\n");
            }
            
            /*
             *std::string resp_data;
             *rpcframe::RpcStatus ret_st = 
             *        client.call("test_method2", "aaaaaaabbbbbbbbccccccc", resp_data, 3);
             *if (rpcframe::RpcStatus::RPC_CB_OK == ret_st) {
             *    //printf("test_method2 back %s\n", resp_data.c_str());
             *}
             *else {
             *    printf("test_method2 call fail %d\n", ret_st);
             *}
             */
        }
    }
    conn_cnt = 1;
    pkg_cnt = 10;
    //round 1, async call all callback will timeout
    printf("round 1\n");
    for (int cnt = 0; cnt < conn_cnt; ++cnt) {
        for (int pcnt = 0; pcnt < pkg_cnt; ++pcnt) {
            std::random_device rd;
            uint32_t len = rd();
            if ( len > 1024 * 100 ) {
                len = len % (1024 *100);
            }
            if ( len <= 0) {
                len = 10;
            }
            if( rpcframe::RpcStatus::RPC_SEND_OK != client.async_call("test_method", std::string(len, '*'), 2, new my_CB())) {
                printf("r1 send fail\n");
            }
            client.async_call("test_method", std::string(len, '*'), 4, NULL);
        }
    }
    sleep(30);
    //round 2 async and sync call, random callback timeout
    conn_cnt = 2;
    pkg_cnt = 20;
    for (int cnt = 0; cnt < conn_cnt; ++cnt) {
        rpcframe::RpcClient client2(ccfg, "test_service");
        for (int pcnt = 0; pcnt < pkg_cnt; ++pcnt) {
            std::random_device rd;
            //random data len
            uint32_t len = rd();
            if ( len > 1024 * 100 ) {
                len = len % (1024 *100);
            }
            //send 10 bytes at least
            if ( len <= 0) {
                len = 10;
            }
            if(rpcframe::RpcStatus::RPC_SEND_OK != client2.async_call("test_method", std::string(len, '*'), 10, new my_CB())) {
                printf("r2 send fail\n");
            }
            
            //set timeout to 3 seconds, server may delay in 0-5 seconds
            std::string resp_data;
            rpcframe::RpcStatus ret_st = 
                    client2.call("test_method1", "aaaaaaabbbbbbbbccccccc", resp_data, 3);
            if (rpcframe::RpcStatus::RPC_CB_OK == ret_st) {
                printf("test_method1 back %s\n", resp_data.c_str());
            }
            else {
                printf("test_method1 call fail %d\n", ret_st);
            }
            
            client2.async_call("test_method", std::string(len, '*'), 3, NULL);
        }
    }

    //send request to async server
    //server will send response in aync way, client side not aware of that.
    rpcframe::RpcClient client_call_async_server(ccfg, "test_service_async");
    std::string resp_data;
    //request a server side async response, server will response after 5 seconds,
    //so we set timeout = 10 seconds
    client_call_async_server.async_call("test_method_async", std::string(23, '*'), 10, new my_CB());
    client_call_async_server.async_call("test_method_async", std::string(20, '*'), 10, NULL);
    client_call_async_server.call("test_method_async", std::string(10, '*'), resp_data, 10);
    printf("async server back: %s\n", resp_data.c_str());
    sleep(3);
    return 0;
}

