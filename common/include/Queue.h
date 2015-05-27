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
    bool pop(T& item, uint32_t ms_val)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        if (queue_.empty() && ms_val > 0) {
            std::chrono::milliseconds ms(ms_val);
            if (cond_.wait_for(mlock, ms) == std::cv_status::timeout)
                return false;
        }
        if (queue_.empty())
            return false;
        item = queue_.front();
        queue_.pop();
        return true;
    }

    void push(const T& item)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        queue_.push(item);
        mlock.unlock();
        cond_.notify_one();
    }

    size_t size()
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        return queue_.size();
        
    }
    Queue()=default;
    Queue(const Queue&) = delete;            // disable copying
    Queue& operator=(const Queue&) = delete; // disable assignment

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

};

#endif
