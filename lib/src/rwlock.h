#pragma once

#include <pthread.h>

class rwlock
{
public:
    inline rwlock()
    {
        pthread_rwlock_init(&_lock, nullptr);
    }

    inline ~rwlock()
    {
        pthread_rwlock_destroy(&_lock);
    }

    inline void read_lock()
    {
        pthread_rwlock_rdlock(&_lock);
    }

    inline void write_lock()
    {
        pthread_rwlock_wrlock(&_lock);
    }

    inline bool try_read_lock()
    {
        return pthread_rwlock_tryrdlock(&_lock) == 0;
    }

    inline bool try_write_lock()
    {
        return pthread_rwlock_trywrlock(&_lock) == 0;
    }

    inline void lock()
    {
        read_lock();
    }

    inline void try_lock()
    {
        try_read_lock();
    }

    inline void unlock()
    {
        pthread_rwlock_unlock(&_lock);
    }

private:
    pthread_rwlock_t _lock;
};

class unique_write_lock {
  public:
    explicit unique_write_lock(rwlock& lock) : lock_(lock) {
        lock_.write_lock();
    }
    ~unique_write_lock() {
        lock_.unlock();
    }
    unique_write_lock()                                   = delete;
    unique_write_lock(const unique_write_lock&)             = delete;
    unique_write_lock& operator =(const unique_write_lock&) = delete;
  private:
    rwlock& lock_;
};

