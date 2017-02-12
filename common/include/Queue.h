/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

namespace rpcframe {

template <typename T>
class Queue
{
public:
    bool pop(T& item, uint32_t ms_val = 0)
    {
        std::unique_lock<std::mutex> mlock(m_mutex);
        //timeout == 0, nonblock
        if (ms_val == 0 && m_queue.empty()) {
            return false;
        }
        while (m_queue.empty() && ms_val > 0) {
            std::chrono::milliseconds ms(ms_val);
            if (m_cond_pop.wait_for(mlock, ms) == std::cv_status::timeout)
                return false;
        }
        item = m_queue.front();
        m_queue.pop();
        mlock.unlock();
        m_cond_push.notify_one();
        return true;
    }

    bool peek(T& item)
    {
        std::unique_lock<std::mutex> mlock(m_mutex);
        if (m_queue.empty()) {
            return false;
        }
        item = m_queue.front();
        return true;
    }

    bool push(const T& item, uint32_t ms_val = 0)
    {
        std::unique_lock<std::mutex> mlock(m_mutex);
        if(m_queue.size() >= m_max_q_len) {
            if(m_dropOldOnFull) {
                m_queue.pop();
            }
            else {
                //timeout == 0, nonblock
                if (ms_val == 0) {
                    return false;
                }
                else {
                    std::chrono::milliseconds ms(ms_val);
                    if (m_cond_push.wait_for(mlock, ms) == std::cv_status::timeout) {
                        return false;
                    }
                }
            }
        }
        m_queue.push(item);
        mlock.unlock();
        m_cond_pop.notify_one();
        return true;
    }

    size_t size()
    {
        std::unique_lock<std::mutex> mlock(m_mutex);
        return m_queue.size();
        
    }
    explicit Queue(uint32_t max_queuelen = 10 * 1024 * 1024)
    : m_max_q_len(max_queuelen)
    , m_dropOldOnFull(false)
    {
    
    };

    void setMaxSize(uint32_t max_size) {
      m_max_q_len = max_size;
    };

    void setDropOnFull(bool drop) {
      m_dropOldOnFull = drop;
    };

    Queue(const Queue&) = delete;            // disable copying
    Queue& operator=(const Queue&) = delete; // disable assignment

private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond_pop;
    std::condition_variable m_cond_push;
    uint32_t m_max_q_len;
    bool m_dropOldOnFull;
};

};
