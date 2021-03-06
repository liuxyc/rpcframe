/*
 * Copyright (c) 2015-2017, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include "gtest/gtest.h"
#include "Queue.h"
#include "SpinLock.h"
#include "RWLock.h"
#include "ThreadPool.h"
#include "util.h"
#include <thread>


class RealTask
{
public:
    std::string name;
    int val;
};

class RealWorker
{
public:
    RealWorker(int val, bool test, std::string names) {
    }
    void run(RealTask *rt) {
        printf("name %s val %d\n", rt->name.c_str(), rt->val);
        delete rt;
    }
};

class RealWorkerSharePtr
{
public:
    RealWorkerSharePtr(int val, bool test, std::string names) {
    }
    void run(std::shared_ptr<RealTask> rt) {
        printf("name %s val %d\n", rt->name.c_str(), rt->val);
    }
};

TEST(ThreadPoolTest, full)
{
    //for raw pointer task
    rpcframe::ThreadPool<RealTask *, RealWorker> tp(10, 666, true, "abc");
    for(auto i = 0; i < 1000; ++i) {
        RealTask *rt = new RealTask;
        rt->name = "real_task_" + std::to_string(i);
        rt->val = i;
        tp.addTask(rt, 100);
    }
    //for shared_ptr Task
    rpcframe::ThreadPool<std::shared_ptr<RealTask>, RealWorkerSharePtr> tpshared(10, 888, true, "abc");
    for(auto i = 0; i < 1000; ++i) {
        std::shared_ptr<RealTask> rt(new RealTask);
        rt->name = "real_task_shared_" + std::to_string(i);
        rt->val = i;
        tpshared.addTask(rt, 100);
    }
    sleep(5);
}


TEST(HashThreadPoolTest, full)
{
    //for raw pointer task
    rpcframe::HashThreadPool<RealTask *, RealWorker> tp(10, 666, true, "abc");
    for(auto i = 0; i < 1000; ++i) {
        RealTask *rt = new RealTask;
        rt->name = "real_task_" + std::to_string(i);
        rt->val = i;
        tp.addTask(rt, std::to_string(rt->val), 100);
    }
    //for shared_ptr Task
    rpcframe::HashThreadPool<std::shared_ptr<RealTask>, RealWorkerSharePtr> tpshared(10, 888, true, "abc");
    for(auto i = 0; i < 1000; ++i) {
        std::shared_ptr<RealTask> rt(new RealTask);
        rt->name = "real_task_shared_" + std::to_string(i);
        rt->val = i;
        tpshared.addTask(rt, std::to_string(rt->val), 100);
    }
    sleep(5);
}

TEST(QueueTest, full)
{
  rpcframe::Queue<int> q_t;
  q_t.push(1);
  q_t.push(2);
  q_t.push(3);
  EXPECT_EQ((size_t)3, q_t.size());
  size_t init_qsize = q_t.size();
  int a = -1;
  for (int i = 1; i < 4; ++i) {
    EXPECT_TRUE(q_t.pop(a, i));
    EXPECT_EQ(i, a);
    EXPECT_EQ(init_qsize - i, q_t.size());
  }

  EXPECT_FALSE(q_t.pop(a, 10));
  std::time_t begin = std::time(nullptr);
  EXPECT_FALSE(q_t.pop(a, 1000));
  EXPECT_TRUE((std::time(nullptr) - begin >= 1));
  EXPECT_FALSE(q_t.pop(a, 0));
}

TEST(QueueTest, block)
{
  rpcframe::Queue<int> q_t(3);
  q_t.push(1);
  q_t.push(2);
  q_t.push(3);
  EXPECT_FALSE(q_t.push(4, 10));
  EXPECT_EQ((size_t)3, q_t.size());
  q_t.setMaxSize(4);
  EXPECT_TRUE(q_t.push(4, 10));
  EXPECT_EQ((size_t)4, q_t.size());
  EXPECT_FALSE(q_t.push(4, 10));
  EXPECT_FALSE(q_t.push(4, 0));
  int a = -1;
  EXPECT_TRUE(q_t.pop(a, 10));
  EXPECT_EQ(a, 1);
  EXPECT_TRUE(q_t.push(4, 10));
}

TEST(QueueTest, thread)
{
  rpcframe::Queue<int> q_t;
  std::thread t1([&q_t](){
      for(int i = 0; i < 10000; ++i) {
        EXPECT_TRUE(q_t.push(1));
      }
      });
  std::thread t2([&q_t](){
      for(int i = 0; i < 10000; ++i) {
        EXPECT_TRUE(q_t.push(0));
      }
      });

  t1.join();
  t2.join();

  EXPECT_EQ((size_t)20000, q_t.size());
  std::thread tp1([&q_t](){
      for(int i = 0; i < 10000; ++i) {
      int v;
        EXPECT_TRUE(q_t.pop(v, 0));
      }
      });
  std::thread tp2([&q_t](){
      for(int i = 0; i < 10000; ++i) {
      int v;
        EXPECT_TRUE(q_t.pop(v, 0));
      }
      });

  tp1.join();
  tp2.join();

  EXPECT_EQ((size_t)0, q_t.size());
}


TEST(SpinLockTest, thread)
{
  rpcframe::SpinLock sp_lock;
  uint32_t number = 0;

  auto thread_func = [&sp_lock, &number](){
    for(int i = 0; i < 100000; ++i) {
      std::lock_guard<rpcframe::SpinLock> lg(sp_lock);
      number++;
    }
  };
  std::vector<std::thread> th_vec;
  for(auto i = 0; i < 10; ++i) {
    th_vec.emplace_back(thread_func);
  }
  for(auto &th: th_vec) {
    th.join();
  }
  EXPECT_EQ(number, 1000000);
}

TEST(RWLockTest, thread)
{
  rpcframe::RWLock rw_lock;
  uint32_t wnumber = 0;
  uint32_t rnumber = 0;

  auto wthread_func = [&rw_lock, &wnumber](){
    for(int i = 0; i < 100000; ++i) {
      rw_lock.lock_write();
      wnumber++;
      rw_lock.unlock();
    }
  };
  auto rthread_func = [&rw_lock, &rnumber](){
    for(int i = 0; i < 100000; ++i) {
      rw_lock.lock_read();
      rw_lock.lock_read();
      rnumber++;
      rw_lock.unlock();
      rw_lock.unlock();
    }
  };

  std::vector<std::thread> wth_vec;
  for(auto i = 0; i < 10; ++i) {
    wth_vec.emplace_back(wthread_func);
  }
  std::vector<std::thread> rth_vec;
  for(auto i = 0; i < 10; ++i) {
    rth_vec.emplace_back(rthread_func);
  }

  for(auto &th: wth_vec) {
    th.join();
  }
  EXPECT_EQ(wnumber, 1000000);

  for(auto &th: rth_vec) {
    th.join();
  }
  EXPECT_NE(rnumber, 1000000);
}

TEST(LogTest, print)
{
  rpcframe::RPC_LOG(rpcframe::RPC_LOG_LEV::DEBUG, "%s", "hello");
  rpcframe::RPC_LOG(rpcframe::RPC_LOG_LEV::INFO, "%s", "hello");
  rpcframe::RPC_LOG(rpcframe::RPC_LOG_LEV::WARNING, "%s", "hello");
  rpcframe::RPC_LOG(rpcframe::RPC_LOG_LEV::ERROR, "%s", "hello");
  rpcframe::RPC_LOG(rpcframe::RPC_LOG_LEV::FATAL, "%s", "hello");
  rpcframe::RPC_LOG(rpcframe::RPC_LOG_LEV::DEBUG, "%s", std::string(4097, 'c').c_str());
}

TEST(UtilTest, GetHost)
{
    std::string myip;
    EXPECT_TRUE(rpcframe::getHostIp(myip));
    EXPECT_EQ(myip, "127.0.1.1");
    myip = "";
    EXPECT_TRUE(rpcframe::getHostIpByName(myip, "localhost"));
    EXPECT_EQ(myip, "127.0.0.1");

}
