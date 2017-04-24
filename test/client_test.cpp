/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include "gtest/gtest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <random>
#include "RpcClient.h"

class success_CB : public rpcframe::RpcClientCallBack 
{
public:
    success_CB()
    {};
    virtual ~success_CB() {};

    virtual void callback(const rpcframe::RpcStatus status, const rpcframe::RawData &response_data) {
        ASSERT_EQ(status, rpcframe::RpcStatus::RPC_SERVER_OK);
        ASSERT_TRUE(response_data.size() != 0);
    }
};
class succ_timeout_CB : public rpcframe::RpcClientCallBack 
{
public:
    succ_timeout_CB()
    {};
    virtual ~succ_timeout_CB() {};

    virtual void callback(const rpcframe::RpcStatus status, const rpcframe::RawData &response_data) {
        ASSERT_TRUE(status == rpcframe::RpcStatus::RPC_CB_TIMEOUT || status == rpcframe::RpcStatus::RPC_SERVER_OK);
        if(status == rpcframe::RpcStatus::RPC_CB_TIMEOUT) {
          printf("async call timeout\n");
        }
    }
};
class succ_disconn_CB : public rpcframe::RpcClientCallBack 
{
public:
    succ_disconn_CB()
    {};
    virtual ~succ_disconn_CB() {};

    virtual void callback(const rpcframe::RpcStatus status, const rpcframe::RawData &response_data) {
        ASSERT_TRUE(status == rpcframe::RpcStatus::RPC_DISCONNECTED || status == rpcframe::RpcStatus::RPC_SERVER_OK);
    }
};
class srv_notfound_CB : public rpcframe::RpcClientCallBack 
{
public:
    srv_notfound_CB()
    {};
    virtual ~srv_notfound_CB() {};

    virtual void callback(const rpcframe::RpcStatus status, const rpcframe::RawData &response_data) {
        ASSERT_EQ(status, rpcframe::RpcStatus::RPC_SRV_NOTFOUND);
    }
};
class method_notfound_CB : public rpcframe::RpcClientCallBack 
{
public:
    method_notfound_CB()
    {};
    virtual ~method_notfound_CB() {};

    virtual void callback(const rpcframe::RpcStatus status, const rpcframe::RawData &response_data) {
        ASSERT_EQ(status, rpcframe::RpcStatus::RPC_METHOD_NOTFOUND);
    }
};
class big_resp_CB : public rpcframe::RpcClientCallBack 
{
public:
    big_resp_CB()
    {};
    virtual ~big_resp_CB() {};

    virtual void callback(const rpcframe::RpcStatus status, const rpcframe::RawData &response_data) {
        ASSERT_EQ(status, rpcframe::RpcStatus::RPC_SERVER_OK);
        ASSERT_EQ(response_data.size(), 1024*1024*40 + 1);
    }
};
class match_size_CB : public rpcframe::RpcClientCallBack 
{
public:
    explicit match_size_CB(size_t size)
    : m_size(size)
    {};
    virtual ~match_size_CB() {};

    virtual void callback(const rpcframe::RpcStatus status, const rpcframe::RawData &response_data) {
        ASSERT_EQ(status, rpcframe::RpcStatus::RPC_SERVER_OK);
        ASSERT_EQ(response_data.size(), m_size + 1);
    }
    size_t m_size;
};
class req_too_large : public rpcframe::RpcClientCallBack 
{
public:
    req_too_large()
    {};
    virtual ~req_too_large() {};

    virtual void callback(const rpcframe::RpcStatus status, const rpcframe::RawData &response_data) {
        ASSERT_EQ(status, rpcframe::RpcStatus::RPC_REQ_TOO_LARGE);
    }
};

TEST(ClientTest, big_req_resp)
{
  auto endp = std::make_pair("localhost", 8801);
  rpcframe::RpcClientConfig ccfg({endp});
  ccfg.setThreadNum(4);
  std::shared_ptr<big_resp_CB> pCB(new big_resp_CB());
  pCB->setShared(true);

  rpcframe::RpcClient client(ccfg, "test_service");
  //async with big resp
  int pkg_cnt = 100;
  std::string req(1024 * 1024 * 2, 'a');
  rpcframe::RawData reqd(req);
  for (int pcnt = 0; pcnt < pkg_cnt; ++pcnt) {
    ASSERT_EQ(rpcframe::RpcStatus::RPC_SEND_OK, client.async_call("test_method_big_resp", reqd, 100, pCB));
  }
  rpcframe::RawData resp;
  ASSERT_EQ(rpcframe::RpcStatus::RPC_SERVER_OK, client.call("test_method_big_resp", reqd, resp, 100));
  ASSERT_EQ(resp.size(), 1024*1024*40 + 1);
  client.waitAllCBDone(5);
}

TEST(ClientTest, unknow_srv_method)
{
  auto endp = std::make_pair("localhost", 8801);
  rpcframe::RpcClientConfig ccfg({endp});
  ccfg.setThreadNum(4);
  std::shared_ptr<srv_notfound_CB> srv_nf_CB(new srv_notfound_CB());
  srv_nf_CB->setShared(true);
  //call not exist service
  rpcframe::RpcClient client_wrong_srv(ccfg, "test_service_not_exist");
  rpcframe::RawData req("abc", 4);
  ASSERT_EQ(rpcframe::RpcStatus::RPC_SEND_OK, client_wrong_srv.async_call("test_method_big_resp_unknow", req, 100, srv_nf_CB));

  rpcframe::RawData resp_wrong_data;
  ASSERT_EQ(rpcframe::RpcStatus::RPC_SRV_NOTFOUND, client_wrong_srv.call("test_method_big_resp_unknow", req, resp_wrong_data, 10));

  std::shared_ptr<method_notfound_CB> method_nf_CB(new method_notfound_CB());
  rpcframe::RpcClient client(ccfg, "test_service");
  ASSERT_EQ(rpcframe::RpcStatus::RPC_SEND_OK, client.async_call("test_method_big_resp_unknow", req, 2, method_nf_CB));
  rpcframe::RawData resp;
  ASSERT_EQ(rpcframe::RpcStatus::RPC_METHOD_NOTFOUND, client.call("test_method_big_resp_unknow", req, resp, 2));

  client.waitAllCBDone(5);
  client_wrong_srv.waitAllCBDone(5);
}


TEST(ClientTest, 3000_sync_async_call)
{
  int pkg_cnt = 1000;
  auto endp = std::make_pair("localhost", 8801);
  rpcframe::RpcClientConfig ccfg({endp});
  ccfg.setThreadNum(4);
  std::shared_ptr<success_CB> pCB(new success_CB());
  pCB->setShared(true);
  //fast async/sync call
  rpcframe::RpcClient newclient(ccfg, "test_service");
  std::string req(1024 * 10, '*');
  rpcframe::RawData reqd(req);
  for (int pcnt = 0; pcnt < pkg_cnt; ++pcnt) {
    ASSERT_EQ(rpcframe::RpcStatus::RPC_SEND_OK, newclient.async_call("test_method_fast_return", 
          reqd, 
          10, 
          pCB));
    ASSERT_EQ(rpcframe::RpcStatus::RPC_SEND_OK, newclient.async_call("test_method_fast_return", 
          reqd,
          10, 
          std::make_shared<success_CB>()));
    rpcframe::RawData resp_data;
    rpcframe::RpcStatus ret_st = newclient.call("test_method_fast_return", reqd, resp_data, 3);
    ASSERT_EQ(rpcframe::RpcStatus::RPC_SERVER_OK, ret_st);
    ASSERT_EQ(0, strcmp(resp_data.data, "my feedback"));
  }
  newclient.waitAllCBDone(5);
}

TEST(ClientTest, small_echo_call)
{
  int pkg_cnt = 32 * 1024;
  auto endp = std::make_pair("localhost", 8801);
  rpcframe::RpcClientConfig ccfg({endp});
  ccfg.setThreadNum(4);
  //fast async/sync call
  rpcframe::RpcClient newclient(ccfg, "test_service");
  for (int pcnt = 1; pcnt <= pkg_cnt; ++pcnt) {
    ccfg.setMaxReqPkgSize(pcnt);
    std::string req(pcnt, 'c');
    std::string reqs(pcnt, 'e');
    rpcframe::RawData reqd(req);
    rpcframe::RawData reqsd(reqs);
    ASSERT_EQ(rpcframe::RpcStatus::RPC_SEND_OK, newclient.async_call("test_method_echo", 
          reqd,
          10, 
          std::make_shared<match_size_CB>(pcnt)));
    rpcframe::RawData resp_data;
    rpcframe::RpcStatus ret_st = newclient.call("test_method_echo", reqsd, resp_data, 5);
    ASSERT_EQ(rpcframe::RpcStatus::RPC_SERVER_OK, ret_st);
    ASSERT_EQ(resp_data.size(), pcnt + 1);
  }
  newclient.waitAllCBDone(5);
}

TEST(ClientTest, hugesize_req)
{
  auto endp = std::make_pair("localhost", 8801);
  rpcframe::RpcClientConfig ccfg({endp});
  ccfg.setThreadNum(4);
  //fast async/sync call
  rpcframe::RpcClient newclient(ccfg, "test_service");
  std::string s1(129 * 1024 * 1024, 'c');
  std::string s2((128 * 1024 * 1024) + 1, 'e');
  rpcframe::RawData req1(s1);
  rpcframe::RawData req2(s2);
  ASSERT_EQ(rpcframe::RpcStatus::RPC_REQ_TOO_LARGE, newclient.async_call("test_method_echo", 
        req1,
        10, 
        std::make_shared<req_too_large>()));
  rpcframe::RawData resp_data;
  rpcframe::RpcStatus ret_st = newclient.call("test_method_echo", req2, resp_data, 10);
  ASSERT_EQ(rpcframe::RpcStatus::RPC_REQ_TOO_LARGE, ret_st);
  newclient.waitAllCBDone(5);
}

TEST(ClientTest, DISABLED_huge_echo_call)
{
  int pkg_cnt = 1024 * 1024;
  auto endp = std::make_pair("localhost", 8801);
  rpcframe::RpcClientConfig ccfg({endp});
  ccfg.setThreadNum(4);
  //fast async/sync call
  rpcframe::RpcClient newclient(ccfg, "test_service");
  for (int pcnt = 1; pcnt <= pkg_cnt; ++pcnt) {
    std::string s1(pcnt, 'c');
    rpcframe::RawData req(s1);
    ASSERT_EQ(rpcframe::RpcStatus::RPC_SEND_OK, newclient.async_call("test_method_echo", 
          req,
          10, 
          std::make_shared<match_size_CB>(pcnt)));
    rpcframe::RawData resp_data;
    rpcframe::RpcStatus ret_st = newclient.call("test_method_echo", req, resp_data, 5);
    ASSERT_EQ(rpcframe::RpcStatus::RPC_SERVER_OK, ret_st);
    ASSERT_EQ(resp_data.size(), pcnt);
  }
  newclient.waitAllCBDone(5);
}

TEST(ClientTest, random_timeout)
{
  auto endp = std::make_pair("localhost", 8801);
  rpcframe::RpcClientConfig ccfg({endp});
  ccfg.setThreadNum(4);
  std::shared_ptr<success_CB> pCB(new success_CB());
  std::shared_ptr<succ_timeout_CB> succ_to_CB(new succ_timeout_CB());
  pCB->setShared(true);
  int conn_cnt = 2;
  int pkg_cnt = 10;
  for (int cnt = 0; cnt < conn_cnt; ++cnt) {
    rpcframe::RpcClient client(ccfg, "test_service");
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

      std::string s1(len, '*');
      rpcframe::RawData req(s1);
      ASSERT_EQ(rpcframe::RpcStatus::RPC_SEND_OK, client.async_call("test_method_5sec_delay", 
            req,
            10, 
            pCB));
      //set timeout to 3 seconds, server may delay in 0-5 seconds
      ASSERT_EQ(rpcframe::RpcStatus::RPC_SEND_OK, client.async_call("test_method_random_delay", 
            req,
            3, 
            succ_to_CB));
      rpcframe::RawData resp_data;
      rpcframe::RpcStatus ret_st = 
        client.call("test_method_random_delay", req, resp_data, 3);
      ASSERT_TRUE(ret_st == rpcframe::RpcStatus::RPC_CB_TIMEOUT || ret_st == rpcframe::RpcStatus::RPC_SERVER_OK);
      if( ret_st == rpcframe::RpcStatus::RPC_SERVER_OK ) {
        ASSERT_EQ(0, strcmp(resp_data.data, "my feedback_random_delay"));
      }
      client.async_call("test_method_random_delay", req, 3, nullptr);
    }
    client.waitAllCBDone(5);
  }
}
    
TEST(ClientTest, server_async_back)
{
  auto endp = std::make_pair("localhost", 8801);
  rpcframe::RpcClientConfig ccfg({endp});
  ccfg.setThreadNum(4);
  std::shared_ptr<success_CB> pCB(new success_CB());
  pCB->setShared(true);
  //send request to async server
  //server will send response in async way, client side not aware of that.
  rpcframe::RpcClient client_call_async_server(ccfg, "test_service_async");
  rpcframe::RawData resp_data;
  //request a server side async response, server will response after 5 seconds,
  //so we set timeout = 10 seconds
  std::string s1(23, '*');
  std::string s2(20, '*');
  std::string s3(10, '*');
  rpcframe::RawData req(s1);
  rpcframe::RawData req2(s2);
  rpcframe::RawData req3(s3);
  ASSERT_EQ(rpcframe::RpcStatus::RPC_SEND_OK, client_call_async_server.async_call("test_method_async", req, 10, pCB));
  ASSERT_EQ(rpcframe::RpcStatus::RPC_SEND_OK, client_call_async_server.async_call("test_method_async", req2, 10, nullptr));
  ASSERT_EQ(rpcframe::RpcStatus::RPC_SERVER_OK, client_call_async_server.call("test_method_async", req3, resp_data, 20));
  ASSERT_EQ(0, strcmp(resp_data.data, "my feedback async"));
  client_call_async_server.waitAllCBDone(5);
}

TEST(ClientTest, concurrent_conn)
{
  auto thread_func = [](){
    auto endp = std::make_pair("127.0.0.1", 8801);
    rpcframe::RpcClientConfig ccfg({endp});
    std::shared_ptr<succ_disconn_CB> pCB(new succ_disconn_CB());
    pCB->setShared(true);
    ccfg.setThreadNum(1);
    std::string s1(23, '*');
    rpcframe::RawData req(s1);
    std::string s3(10, '*');
    rpcframe::RawData req1(s3);
    for(auto i = 0; i < 10; ++i) {
      rpcframe::RpcClient client(ccfg, "test_service");
      ASSERT_EQ(rpcframe::RpcStatus::RPC_SEND_OK, client.async_call("test_method_fast_return", req, 10, pCB));
      rpcframe::RawData resp_data;
      rpcframe::RpcStatus ret_st = client.call("test_method_echo", req1, resp_data, 20);
      ASSERT_TRUE(ret_st == rpcframe::RpcStatus::RPC_DISCONNECTED || ret_st == rpcframe::RpcStatus::RPC_SERVER_OK);
    }
  };

  std::vector<std::thread> thread_vec;
  for(auto i = 0; i < 100; ++i) {
    thread_vec.emplace_back(thread_func);
  }

  for(auto &th: thread_vec) {
    th.join();
  }
}

TEST(ClientTest, concurrent_req)
{
  auto endp = std::make_pair("127.0.0.1", 8801);
  auto endp1 = std::make_pair("127.0.0.1", 8802);
  rpcframe::RpcClientConfig ccfg({endp, endp1});
  std::shared_ptr<success_CB> pCB(new success_CB());
  pCB->setShared(true);
  ccfg.setThreadNum(4);
  rpcframe::RpcClient client(ccfg, "test_service");
  auto thread_func = [&client, &pCB](){
    for(auto i = 0; i < 100; ++i) {
      if(i == 95) {
          client.reloadEndpoints({{"127.0.0.1", 8803}, {"127.0.0.1", 8804}, {"127.0.0.1", 8801}});
      }
      std::string s1(i, '*');
      rpcframe::RawData req(s1);
      ASSERT_EQ(rpcframe::RpcStatus::RPC_SEND_OK, client.async_call("test_method_fast_return", req, 10, pCB));
      rpcframe::RawData resp_data;
      rpcframe::RpcStatus ret_st = client.call("test_method_echo", req, resp_data, 20);
      ASSERT_TRUE(ret_st == rpcframe::RpcStatus::RPC_DISCONNECTED || ret_st == rpcframe::RpcStatus::RPC_SERVER_OK);
    }
  };

  std::vector<std::thread> thread_vec;
  for(auto i = 0; i < 1; ++i) {
    thread_vec.emplace_back(thread_func);
  }
  for(auto &th: thread_vec) {
    th.join();
  }
}
