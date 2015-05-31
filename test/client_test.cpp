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
            printf("client cb return %d, got %s\n", status, response_data.c_str());
        }
    }
    
};
int main()
{
    int conn_cnt = 1;
    int pkg_cnt = 10;
    auto endp = std::make_pair("127.0.0.1", 8801);
    rpcframe::RpcClientConfig ccfg(endp);

    rpcframe::RpcClient client(ccfg, "test_service");
    //round 1, async call
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
                printf("send fail\n");
            }
            client.async_call("test_method", std::string(len, '*'), 4, NULL);
        }
    }
    sleep(30);
    //round 2 with sync call
    conn_cnt = 2;
    pkg_cnt = 10;
    for (int cnt = 0; cnt < conn_cnt; ++cnt) {
        rpcframe::RpcClient client2(ccfg, "test_service");
        for (int pcnt = 0; pcnt < pkg_cnt; ++pcnt) {
            std::random_device rd;
            uint32_t len = rd();
            if ( len > 1024 * 100 ) {
                len = len % (1024 *100);
            }
            if ( len <= 0) {
                len = 10;
            }
            if(rpcframe::RpcStatus::RPC_SEND_OK != client2.async_call("test_method", std::string(len, '*'), 10, new my_CB())) {
                printf("send fail\n");
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
    return 0;
}

