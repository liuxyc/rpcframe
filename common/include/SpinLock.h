/*
 * Copyright (c) 2015-2016, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include <pthread.h>
namespace rpcframe {
class SpinLock
{
  public:
    SpinLock() { 
      pthread_spin_init(&m_lock, 0); 
    }
    ~SpinLock() { 
      pthread_spin_destroy(&m_lock); 
    }
    void lock() { 
      pthread_spin_lock(&m_lock); 
    }
    bool try_lock() { 
      return pthread_spin_trylock(&m_lock) == 0; 
    }
    void unlock() { 
      pthread_spin_unlock(&m_lock); 
    }
  private:
    pthread_spinlock_t m_lock;
}; 

};
