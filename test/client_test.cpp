/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include "RpcClient.h"

class my_CB : public rpcframe::RpcClientCallBack 
{
public:
    my_CB(){};
    virtual ~my_CB() {};

    virtual void callback(const rpcframe::RpcClientCallBack::RpcCBStatus status, const std::string &response_data) {
        printf("client cb return %d, got %s\n", status, response_data.c_str());
    }
    
};
int main()
{
    int conn_cnt = 1;
    int pkg_cnt = 5;
    auto endp = std::make_pair("127.0.0.1", 8801);
    rpcframe::RpcClientConfig ccfg(endp);
    rpcframe::RpcClient client(ccfg, "test_service");

    for (int cnt = 0; cnt < conn_cnt; ++cnt) {
        for (int pcnt = 0; pcnt < pkg_cnt; ++pcnt) {
            client.async_call("test_method", "aaaaaaabbbbbbbbccccccc", 3, new my_CB());
            std::string resp_data;
            if (client.call("test_method1", "aaaaaaabbbbbbbbccccccc", resp_data, 3)) {
                printf("test_method1 back %s\n", resp_data.c_str());
            }
        }
    }
    return 0;
}

