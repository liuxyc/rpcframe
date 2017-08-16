/*
 * Copyright (c) 2015-2017, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include "Queue.h"
#include <thread>
#include <atomic>
#include <utility>
#include <sys/prctl.h>
#include <signal.h>
#include <util.h>
#include <sstream>

namespace rpcframe
{


// WorkerT must have method void run(TaskT *)
template <typename TaskT, typename WorkerT>
class ThreadPool
{
public:
    typedef Queue<TaskT> TaskQueue;
//ThreadPool::Worker
    class Worker
    {
    public:
        Worker(TaskQueue *q, WorkerT *real_worker, uint32_t queuetimeout)
        : m_q(q)
        , m_stop(false)
        , m_thread(new std::thread(&Worker::run, this))
        , m_q_timeout(queuetimeout)
        , m_real_worker(real_worker)
        {
        }
        ~Worker()
        {
            m_thread->join();
            delete m_real_worker;
        }

        void stop()
        {
            m_stop.store(true);
        }

        WorkerT *getRealWorker()
        {
            return m_real_worker;
        }

        void run()
        {
            sigset_t set;
            sigemptyset(&set);
            sigaddset(&set, SIGINT);
            int s = pthread_sigmask(SIG_BLOCK, &set, NULL);
            std::stringstream ss;
            ss << std::this_thread::get_id();
            if (s != 0) {
                RPC_LOG(RPC_LOG_LEV::ERROR, "thread %s block SIGINT fail", ss.str().c_str());
            }
            prctl(PR_SET_NAME, "ThreadPoolWorker", 0, 0, 0); 
            while(!m_stop.load()) {
                TaskT task;
                if (m_q->pop(task, m_q_timeout)) {
                    m_real_worker->run(task);
                }
            }
        }

    private:
        TaskQueue *m_q;
        std::atomic<bool> m_stop;
        std::unique_ptr<std::thread> m_thread;
        uint32_t m_q_timeout;
        WorkerT *m_real_worker;
    };

//ThreadPool
    template<typename...Args>
    ThreadPool(size_t thread_num, const Args& ... WArgs)
    {
        for(auto i = 0UL; i < thread_num; ++i) {
            WorkerT *real_worker = new WorkerT(WArgs...);
            m_worker_threads.emplace_back(new Worker(&m_taskQ, real_worker, 100));
        }
    }
    ~ThreadPool()
    {
        for(auto &w: m_worker_threads) {
            w->stop();
        }
    }

    size_t getTaskQSize()
    {
        return m_taskQ.size();
    }

    void setTaskQSize(size_t qmaxsize)
    {
        m_taskQ.setMaxSize(qmaxsize);
    }

    bool addTask(TaskT task, uint32_t timeout = 0) 
    {
        return m_taskQ.push(task, timeout);
    }

    void getWorkers(std::vector<WorkerT *> &real_workers) 
    {
        for(auto &w : m_worker_threads) {
            real_workers.push_back(w->getRealWorker());
        }
    }

private:
    TaskQueue m_taskQ;
    std::vector<std::unique_ptr<Worker>> m_worker_threads;
};


}
