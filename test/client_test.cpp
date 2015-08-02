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
    {};
    virtual ~my_CB() {};

    virtual void callback(const rpcframe::RpcStatus status, const std::string &response_data) {
        if (status != rpcframe::RpcStatus::RPC_SERVER_OK) {
            printf("client cb return %d\n", status);
        }
        else {
            //printf("client cb return %d, got %s\n", status, response_data.c_str());
            printf("client cb return %d, got data len %lu\n", status, response_data.length());
        }

    }
    
};

int main()
{
    int pkg_cnt = 2;
    auto endp = std::make_pair("localhost", 8801);
    rpcframe::RpcClientConfig ccfg(endp);
    ccfg.setThreadNum(4);
    my_CB *pCB = new my_CB();

    rpcframe::RpcClient client(ccfg, "test_service");
    //async with big resp
    for (int pcnt = 0; pcnt < pkg_cnt; ++pcnt) {
        if(rpcframe::RpcStatus::RPC_SEND_OK != client.async_call("test_method_big_resp", 
                                                                 std::string(1024 * 10, '*'), 
                                                                 10, 
                                                                 pCB)) 
        {
            printf("send fail\n");
        }
    }
    client.async_call("test_method_big_resp_unknow", "unknow", 2, pCB);

    //call not exist service
    rpcframe::RpcClient client_wrong_srv(ccfg, "test_service_not_exist");
    client_wrong_srv.async_call("test_method_big_resp_unknow", "unknow", 2, pCB);
    std::string resp_wrong_data;
    if (rpcframe::RpcStatus::RPC_SRV_NOTFOUND == client_wrong_srv.call("test_method_big_resp_unknow", "unknow", resp_wrong_data, 2)) {
        printf("call unknow serivce back\n");
    }

    int conn_cnt = 1;
    pkg_cnt = 5000;
    //fast async/sync call
    rpcframe::RpcClient newclient(ccfg, "test_service");
    for (int cnt = 0; cnt < conn_cnt; ++cnt) {
        for (int pcnt = 0; pcnt < pkg_cnt; ++pcnt) {
            if(rpcframe::RpcStatus::RPC_SEND_OK != newclient.async_call("test_method2", 
                                                                std::string(1024 * 10, '*'), 
                                                                10, 
                                                                pCB)) 
            {
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

    sleep(5);
    pkg_cnt = 10;
    //round 1, async call all callback will timeout
    printf("round 1\n");
    for (int pcnt = 0; pcnt < pkg_cnt; ++pcnt) {
        std::random_device rd;
        uint32_t len = rd();
        if ( len > 1024 * 100 ) {
            len = len % (1024 *100);
        }
        if ( len <= 0) {
            len = 10;
        }
        if( rpcframe::RpcStatus::RPC_SEND_OK != client.async_call("test_method", 
                                                                  std::string(len, '*'), 
                                                                  2, 
                                                                  pCB)) 
        {
            printf("r1 send fail\n");
        }
        client.async_call("test_method", std::string(len, '*'), 3, NULL);
    }
    sleep(30);
    //round 2 async and sync call, random callback timeout
    conn_cnt = 2;
    pkg_cnt = 10;
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
            if(rpcframe::RpcStatus::RPC_SEND_OK != client2.async_call("test_method", 
                                                                      std::string(len, '*'), 
                                                                      10, 
                                                                      pCB)) 
            {
                printf("r2 send fail\n");
            }
            
            //set timeout to 3 seconds, server may delay in 0-5 seconds
            std::string resp_data;
            rpcframe::RpcStatus ret_st = 
                    client2.call("test_method1", "aaaaaaabbbbbbbbccccccc", resp_data, 3);
            if (rpcframe::RpcStatus::RPC_SERVER_OK == ret_st) {
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
    client_call_async_server.async_call("test_method_async", std::string(23, '*'), 10, pCB);
    client_call_async_server.async_call("test_method_async", std::string(20, '*'), 10, NULL);
    client_call_async_server.call("test_method_async", std::string(10, '*'), resp_data, 10);
    printf("async server back: %s\n", resp_data.c_str());
    sleep(10);
    for(uint32_t i = 0; i < 1000; ++i) {
        std::random_device rd;
        uint32_t len = rd();
        if ( len > 1024 * 100 ) {
            len = len % (1024 *100);
        }
        if ( len <= 0) {
            len = 10;
        }
        if( rpcframe::RpcStatus::RPC_SEND_OK != client.async_call("test_method2", 
                                                                  std::string(len, '*'), 
                                                                  2, 
                                                                  pCB)) 
        {
            printf("r1 send fail\n");
        }
        client.async_call("test_method2", std::string(len, '*'), 4, NULL);
    }
    sleep(10);
    return 0;
}

