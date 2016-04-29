/*
 * Copyright (c) 2014-2015, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include <pthread.h>
namespace rpcframe {
class RWLock
{
  public:
    RWLock()
    {
      pthread_rwlock_init(&m_lock, NULL);
    }
    ~RWLock()
    {
      pthread_rwlock_destroy(&m_lock);
    }
    void lock_write()
    {
      pthread_rwlock_wrlock(&m_lock);
    }
    void lock_read()
    {
      pthread_rwlock_rdlock(&m_lock);
    }
    void unlock()
    {
      pthread_rwlock_unlock(&m_lock);
    }
  private:
    pthread_rwlock_t m_lock;
};

class ReadLockGuard
{
  public:
    explicit ReadLockGuard(RWLock& lock) 
    : m_lock(lock)
    {
      m_lock.lock_read();
    }

    ~ReadLockGuard()
    {
      m_lock.unlock();
    }

  private:
    RWLock& m_lock;
};

class WriteLockGuard
{
  public:
    explicit WriteLockGuard(RWLock& lock) 
    : m_lock(lock)
    {
      m_lock.lock_write();
    }

    ~WriteLockGuard()
    {
      m_lock.unlock();
    }
  private:
    RWLock& m_lock;
};

};
