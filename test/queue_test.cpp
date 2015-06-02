#include "gtest/gtest.h"
#include "Queue.h"

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
    EXPECT_FALSE(q_t.pop(a, 10));
}

TEST(QueueTest, thread)
{
    rpcframe::Queue<int> q_t;
    std::thread t1([&q_t](){
                for(int i = 0; i < 10000; ++i) {
                    q_t.push(1);
                }
            });
    std::thread t2([&q_t](){
                for(int i = 0; i < 10000; ++i) {
                    q_t.push(0);
                }
            });

    t1.join();
    t2.join();

    EXPECT_EQ((size_t)20000, q_t.size());
    std::thread tp1([&q_t](){
                for(int i = 0; i < 10000; ++i) {
                    int v;
                    q_t.pop(v, 0);
                }
            });
    std::thread tp2([&q_t](){
                for(int i = 0; i < 10000; ++i) {
                    int v;
                    q_t.pop(v, 0);
                }
            });

    tp1.join();
    tp2.join();

    EXPECT_EQ((size_t)0, q_t.size());
}
