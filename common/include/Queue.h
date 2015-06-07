/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#ifndef RPCFRAME_QUEUE_
#define RPCFRAME_QUEUE_

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace rpcframe {

template <typename T>
class Queue
{
public:
    bool pop(T& item, uint32_t ms_val = 0)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        //timeout == 0, nonblock
        if (ms_val == 0 && queue_.empty()) {
            return false;
        }
        while (queue_.empty() && ms_val > 0) {
            std::chrono::milliseconds ms(ms_val);
            if (cond_pop.wait_for(mlock, ms) == std::cv_status::timeout)
                return false;
        }
        item = queue_.front();
        queue_.pop();
        mlock.unlock();
        cond_push.notify_one();
        return true;
    }

    bool peak(T& item)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        item = queue_.front();
        return true;
    }

    bool push(const T& item, uint32_t ms_val = 0)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        //timeout == 0, nonblock
        if (ms_val == 0 && queue_.size() >= m_max_q_len) {
            return false;
        }
        while (queue_.size() >= m_max_q_len && ms_val > 0) {
            std::chrono::milliseconds ms(ms_val);
            if (cond_push.wait_for(mlock, ms) == std::cv_status::timeout)
                return false;
        }
        queue_.push(item);
        mlock.unlock();
        cond_pop.notify_one();
        return true;
    }

    size_t size()
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        return queue_.size();
        
    }
    Queue(uint32_t max_queue_len = 100000000)
    : m_max_q_len(max_queue_len)
    {
    
    };
    Queue(const Queue&) = delete;            // disable copying
    Queue& operator=(const Queue&) = delete; // disable assignment

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_pop;
    std::condition_variable cond_push;
    uint32_t m_max_q_len;
};

};

#endif
