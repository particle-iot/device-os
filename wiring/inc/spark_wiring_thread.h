/**
 ******************************************************************************
 * @file    spark_wiring_thread.h
 * @authors Matthew McGowan
 * @date    03 March 2015
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef SPARK_WIRING_THREAD_H
#define SPARK_WIRING_THREAD_H

#if PLATFORM_THREADING

#include "concurrent_hal.h"
#include <stddef.h>
#include <mutex>
#include <functional>
#include <type_traits>
#include <memory>

typedef std::function<os_thread_return_t(void)> wiring_thread_fn_t;

class SingleThreadedSection {
public:
    SingleThreadedSection() {
        os_thread_scheduling(false, NULL);
    }

    ~SingleThreadedSection() {
        os_thread_scheduling(true, NULL);
    }
};

#define SINGLE_THREADED_SECTION()  SingleThreadedSection __cs;

#define SINGLE_THREADED_BLOCK() for (bool __todo = true; __todo; ) for (SingleThreadedSection __cs; __todo; __todo=0)
#define WITH_LOCK(lock) for (bool __todo = true; __todo;) for (std::lock_guard<decltype(lock)> __lock((lock)); __todo; __todo=0)
#define TRY_LOCK(lock) for (bool __todo = true; __todo; ) for (std::unique_lock<typename std::remove_reference<decltype(lock)>::type> __lock##lock((lock), std::try_to_lock); __todo &= bool(__lock##lock); __todo=0)

#else
#define SINGLE_THREADED_SECTION()
#define SINGLE_THREADED_BLOCK()
#define WITH_LOCK(x)
#define TRY_LOCK(x)
#endif

#if PLATFORM_THREADING

class Thread
{
private:
    struct Data {
        std::unique_ptr<wiring_thread_fn_t> wrapper;
        os_thread_t handle;
        os_thread_fn_t func;
        void* func_param;
        volatile bool started;
        volatile bool exited;

        Data() :
            handle(OS_THREAD_INVALID_HANDLE),
            func(nullptr),
            func_param(nullptr),
            started(false),
            exited(false) {
        }
    };

    // The thread context is allocated dynamically in order to keep all pointers to it valid, even if
    // the original wrapper object has been moved and subsequently destroyed
    std::unique_ptr<Data> d_;

public:
    Thread()
    {
    }

    Thread(const char* name, os_thread_fn_t function, void* function_param=NULL,
            os_thread_prio_t priority=OS_THREAD_PRIORITY_DEFAULT, size_t stack_size=OS_THREAD_STACK_SIZE_DEFAULT)
        : d_(new(std::nothrow) Data)
    {
        if (!d_) {
            goto error;
        }
        d_->func = function;
        d_->func_param = function_param;
        if (os_thread_create(&d_->handle, name, priority, &Thread::run, d_.get(), stack_size) != 0) {
            goto error;
        }
        while (!d_->started) {
            os_thread_yield();
        }
        return;
    error:
        d_.reset();
    }

    Thread(const char *name, wiring_thread_fn_t function,
            os_thread_prio_t priority=OS_THREAD_PRIORITY_DEFAULT, size_t stack_size=OS_THREAD_STACK_SIZE_DEFAULT)
        : d_(new(std::nothrow) Data)
    {
        if (!d_) {
            goto error;
        }
        d_->wrapper.reset(new(std::nothrow) wiring_thread_fn_t(std::move(function)));
        if (!d_->wrapper) {
            goto error;
        }
        if (os_thread_create(&d_->handle, name, priority, &Thread::run, d_.get(), stack_size) != 0) {
            goto error;
        }
        while (!d_->started) {
            os_thread_yield();
        }
        return;
    error:
        d_.reset();
    }

    Thread(Thread&& thread)
        : d_(std::move(thread.d_))
    {
    }

    ~Thread()
    {
        dispose();
    }

    void dispose()
    {
        if (!isValid())
            return;

        // We shouldn't dispose of current thread
        if (isCurrent())
            return;

        if (!d_->exited) {
            join();
        }

        os_thread_cleanup(d_->handle);

        d_.reset();
    }

    bool join()
    {
        return isValid() && os_thread_join(d_->handle)==0;
    }

    bool cancel()
    {
        return isValid() && os_thread_exit(d_->handle)==0;
    }

    bool is_valid() // Deprecated
    {
        return isValid();
    }

    bool isValid() const
    {
        // TODO should this also check xTaskIsTaskFinished as well?
        return (bool)d_;
    }

    bool is_current() // Deprecated
    {
        return isCurrent();
    }

    bool isCurrent() const
    {
        return isValid() && os_thread_is_current(d_->handle);
    }

    Thread& operator=(Thread&& thread)
    {
        d_ = std::move(thread.d_);
        return *this;
    }

private:

    static os_thread_return_t run(void* param) {
        Data* th = (Data*)param;
        th->started = true;
        if (th->func) {
            (*(th->func))(th->func_param);
        } else if (th->wrapper) {
            (*(th->wrapper))();
        }
        th->exited = true;
        os_thread_exit(nullptr);
    }
};

class Mutex
{
    os_mutex_t handle_;
public:
    /**
     * Creates a shared mutex from an existing handle.
     * This is mainly used to share mutexes between dynamically linked modules.
     */
    Mutex(os_mutex_t handle) : handle_(handle) {}

    /**
     * Creates a new mutex.
     */
    Mutex() : handle_(nullptr)
    {
        os_mutex_create(&handle_);
    }

    void dispose()
    {
        if (handle_) {
            os_mutex_destroy(handle_);
            handle_ = nullptr;
        }
    }

    void lock() { os_mutex_lock(handle_); }
    bool trylock() { return os_mutex_trylock(handle_)==0; }
    void unlock() { os_mutex_unlock(handle_); }

};


class RecursiveMutex
{
    os_mutex_recursive_t handle_;
public:
    /**
     * Creates a shared mutex.
     */
    RecursiveMutex(os_mutex_recursive_t handle) : handle_(handle) {}

    RecursiveMutex() : handle_(nullptr)
    {
        os_mutex_recursive_create(&handle_);
    }

    void dispose()
    {
        if (handle_) {
            os_mutex_recursive_destroy(handle_);
            handle_ = nullptr;
        }
    }

    void lock() { os_mutex_recursive_lock(handle_); }
    bool trylock() { return os_mutex_recursive_trylock(handle_)==0; }
    void unlock() { os_mutex_recursive_unlock(handle_); }

};

#endif // PLATFORM_THREADING

namespace particle {

// Class implementing a dummy concurrency policy
class NoConcurrency {
public:
    struct Lock {
    };

    Lock lock() const {
        return Lock();
    }

    void unlock(Lock) const {
    }
};

} // namespace particle

#endif  /* SPARK_WIRING_THREAD_H */

