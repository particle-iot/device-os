/**
 ******************************************************************************
 * @file    concurrency_hal.cpp
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

#include "concurrent_hal.h"
#include "static_assert.h"
#include "delay_hal.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <stdexcept>

/**
 * Creates a new thread.
 * @param thread            Receives the created thread handle. Will be set to NULL if the thread cannot be created.
 * @param name              The name of the thread. May be used if the underlying RTOS supports it. Can be null.
 * @param priority          The thread priority. It's best to stick to a small range of priorities, e.g. +/- 7.
 * @param fun               The function to execute in a separate thread.
 * @param thread_param      The parameter to pass to the thread function.
 * @param stack_size        The size of the stack to create. The stack is allocated on the heap.
 * @return an error code. 0 if the thread was successfully created.
 */
os_result_t os_thread_create(os_thread_t* thread, const char* name, os_thread_prio_t priority, os_thread_fn_t fun, void* thread_param, size_t stack_size)
{
    auto *pThread = new std::thread(fun, thread_param);
    *thread = pThread;
    return 0;
}


/**
 * Determines if the given thread is the one executing.
 * @param   The thread to test.
 * @return {@code true} if the thread given is the one currently executing. {@code false} otherwise.
 */
bool os_thread_is_current(os_thread_t thread)
{
    auto *pThread = static_cast<std::thread*>(thread);
    return pThread->get_id()==std::this_thread::get_id();
}


os_result_t os_thread_yield(void)
{
    auto zero = std::chrono::milliseconds::zero();
    std::this_thread::sleep_for(zero);
    return 0;
}

/**
 * Determines if the thread stack is still within the allocated region.
 * @param thread    The thread to check
 * @return      {@code true} if the thread is still within the allocated stack region.
 *  {@code false} if the thread has gone through the bottom of the stack. Depending upon where the stack was
 *  allocated this may cause corruption of system or user data. As a precaution, the device should be reset unless
 * measures have been taken to ensure the region of memory the stack grows into is unused (such as overallocating the stack size.)
 */
bool os_thread_current_within_stack()
{
    return true;
}

/**
 * Waits indefinitely for the given thread to finish.
 * @param thread    The thread to wait for.
 * @return 0 if the thread has successfully terminated. non-zero if the thread handle is not valid.
 */
os_result_t os_thread_join(os_thread_t thread)
{
    auto *pThread = static_cast<std::thread*>(thread);
    pThread->join();
    return 0;
}

/**
 * Cleans up resources used by a terminated thread.
 * @param thread    The thread to clean up.
 * @return 0 on success.
 */
os_result_t os_thread_cleanup(os_thread_t thread)
{
    auto *pThread = static_cast<std::thread*>(thread);
    delete pThread;
    return 0;
}

/**
 * Delays the current task until a specified time to set up periodic tasks
 * @param previousWakeTime The time the thread last woke up.  May not be NULL.
 *                         Set to the current time on first call. Will be updated
 *                         when the task wakes up
 * @param timeIncrement    The cycle time period
 * @return 0 on success. 1 if previousWakeTime is NULL
 */
os_result_t os_thread_delay_until(system_tick_t *previousWakeTime, system_tick_t timeIncrement)
{
    // FIXME: not accurate but close enough for now
    auto t = std::chrono::milliseconds(timeIncrement);
    std::this_thread::sleep_for(t);
    return 0;
}

/* Thread safe queue
 * From https://juanchopanzacpp.wordpress.com/2013/02/26/concurrent-queue-c11/
 *
 * Copyright (c) 2013, Juan Palacios <juan.palacios.puyana@gmail.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
template <typename T>
class Queue
{
 public:
 
  bool pop(T& item, system_tick_t timeout = CONCURRENT_WAIT_FOREVER)
  {
    std::unique_lock<std::mutex> mlock(mutex_);
    if (queue_.empty() && timeout != 0)
    {
        if (timeout == CONCURRENT_WAIT_FOREVER) {
            cond_.wait(mlock);
        } else {
            auto t = std::chrono::milliseconds(timeout);
            cond_.wait_for(mlock, t);
        }
    }
    if (queue_.empty()) {
        return false;
    } else {
        item = queue_.front();
        queue_.pop();
        return true;
    }
  }
 
  void push(const T& item)
  {
    std::unique_lock<std::mutex> mlock(mutex_);
    queue_.push(item);
    mlock.unlock();
    cond_.notify_one();
  }
 
 private:
  std::queue<T> queue_;
  std::mutex mutex_;
  std::condition_variable cond_;
};

typedef Queue<void *> PtrQueue;

int os_queue_create(os_queue_t* queue, size_t item_size, size_t item_count)
{
    // FIXME: ignore item_size and item_count
    if (item_size != sizeof(void*)) {
        throw std::runtime_error("os_queue_create only supports pointers on this platform");
    }
    auto *pQueue = new PtrQueue;
    *queue = pQueue;
    return 0;
}

int os_queue_put(os_queue_t queue, const void* item, system_tick_t delay)
{
    auto *pQueue = static_cast<PtrQueue*>(queue);
    // item is a point to a item_size bytes of memory.
    // Here we assume it's a pointer to a pointer that we need to push
    auto *pItem = static_cast<void* const*>(item);
    // delay is ignored since the queue grows to accomodate more items
    pQueue->push(*pItem);
    return 0;
}

int os_queue_take(os_queue_t queue, void* item, system_tick_t delay)
{
    auto *pQueue = static_cast<PtrQueue*>(queue);
    // item is a point to a item_size bytes of memory.
    // Here we assume it's a pointer to the pointer that we need to pop
    auto *pItem = static_cast<void**>(item);
    return !pQueue->pop(*pItem, delay);
}

void os_queue_destroy(os_queue_t queue)
{
    auto *pQueue = static_cast<PtrQueue*>(queue);
    delete pQueue;
}

int os_mutex_create(os_mutex_t* mutex)
{
    auto *pMutex = new std::mutex();
    *mutex = pMutex;
    return 0;
}

int os_mutex_destroy(os_mutex_t mutex)
{
    auto *pMutex = static_cast<std::mutex*>(mutex);
    delete pMutex;
    return 0;
}

int os_mutex_lock(os_mutex_t mutex)
{
    auto *pMutex = static_cast<std::mutex*>(mutex);
    pMutex->lock();
    return 0;
}

int os_mutex_trylock(os_mutex_t mutex)
{
    auto *pMutex = static_cast<std::mutex*>(mutex);
    return !pMutex->try_lock();
}


int os_mutex_unlock(os_mutex_t mutex)
{
    auto *pMutex = static_cast<std::mutex*>(mutex);
    pMutex->unlock();
    return 0;
}

int os_mutex_recursive_create(os_mutex_recursive_t* mutex)
{
    auto *pMutex = new std::recursive_mutex();
    *mutex = pMutex;
    return 0;
}

int os_mutex_recursive_destroy(os_mutex_recursive_t mutex)
{
    auto *pMutex = static_cast<std::recursive_mutex*>(mutex);
    delete pMutex;
    return 0;
}

int os_mutex_recursive_lock(os_mutex_recursive_t mutex)
{
    auto *pMutex = static_cast<std::recursive_mutex*>(mutex);
    pMutex->lock();
    return 0;
}

int os_mutex_recursive_trylock(os_mutex_recursive_t mutex)
{
    auto *pMutex = static_cast<std::recursive_mutex*>(mutex);
    return !pMutex->try_lock();
}

int os_mutex_recursive_unlock(os_mutex_recursive_t mutex)
{
    auto *pMutex = static_cast<std::recursive_mutex*>(mutex);
    pMutex->unlock();
    return 0;
}

void os_thread_scheduling(bool enabled, void* reserved)
{
    static std::recursive_mutex schedulingMutex;

    if (enabled) {
        schedulingMutex.unlock();
    } else {
        schedulingMutex.lock();
    }
}

// From https://gist.github.com/sguzman/9594227
class Semaphore
{
public:

    Semaphore(int count_ = 0) : count{count_}
    {}

    void notify()
    {
        std::unique_lock<std::mutex> lck(mtx);
        ++count;
        cv.notify_one();
    }

    bool wait(system_tick_t timeout = CONCURRENT_WAIT_FOREVER) {
        std::unique_lock<std::mutex> lck(mtx);
        if (count == 0 && timeout != 0) {
            if (timeout == CONCURRENT_WAIT_FOREVER) {
                while(count == 0)
                {
                    cv.wait(lck);
                }
            } else {
                auto t = std::chrono::milliseconds(timeout);
                // Don't protect against spurrious wakeups since the caller
                // can handle when the semaphore isn't signaled
                cv.wait_for(lck, t);
            }
        }

        if (count > 0) {
            --count;
            return true;
        } else {
            return false;
        }
    }

private:

    std::mutex mtx;
    std::condition_variable cv;
    int count;
};

int os_semaphore_create(os_semaphore_t* semaphore, unsigned max, unsigned initial)
{
    // FIXME: ignore max for now
    auto *pSemaphore = new Semaphore(initial);
    *semaphore = pSemaphore;
    return 0;
}

int os_semaphore_destroy(os_semaphore_t semaphore)
{
    auto *pSemaphore = static_cast<Semaphore*>(semaphore);
    delete pSemaphore;
    return 0;
}

int os_semaphore_take(os_semaphore_t semaphore, system_tick_t timeout, bool reserved)
{
    auto *pSemaphore = static_cast<Semaphore*>(semaphore);
    return !pSemaphore->wait(timeout);
}

int os_semaphore_give(os_semaphore_t semaphore, bool reserved)
{
    auto *pSemaphore = static_cast<Semaphore*>(semaphore);
    pSemaphore->notify();
    return 0;
}
