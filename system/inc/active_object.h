/**
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


#pragma once

#if PLATFORM_THREADING

#include <functional>
#include <mutex>
#include <thread>
#include <future>
#include "channel.h"
#include "concurrent_hal.h"
#include "interrupts_hal.h"
#include "debug.h"

/**
 * Configuratino data for an active object.
 */
struct ActiveObjectConfiguration
{
    /**
     * Function to call when there are no objects to process in the queue.
     */
    typedef std::function<void(void)> background_task_t;

    /**
     * The function to run when there is nothing else to do.
     */
    background_task_t background_task;
    size_t stack_size;

    /**
     * Time to wait for a message in the queue. This governs how often the
     * background task is executed.
     */
    unsigned take_wait;

    /**
     * How long to wait to put items in the queue before giving up.
     */
    unsigned put_wait;

    /**
     * The message capacity of the queue.
     */
    uint16_t queue_size;

    /**
     * Number of preallocated task objects than can be enqueued from an ISR.
     */
    uint16_t isr_task_pool_size;

public:
    ActiveObjectConfiguration(background_task_t task, unsigned take_wait_, unsigned put_wait_, uint16_t queue_size_,
                uint16_t isr_task_pool_size, size_t stack_size_ = 0) :
            background_task(task),
            stack_size(stack_size_),
            take_wait(take_wait_),
            put_wait(put_wait_),
            queue_size(queue_size_),
            isr_task_pool_size(isr_task_pool_size) {
    }
};

/**
 * A message passed to an active object.
 */
class Message
{

public:
    Message() {}
    virtual void operator()()=0;
    virtual ~Message() {}
};

/**
 * Abstract task. Subclasses must define invoke() and task_complete()
 */
template <typename T, typename C>
class AbstractTask : public Message
{
protected:
    /**
     * The function to invoke to retrieve the future result.
     */
    std::function<T(void)> work;

public:
    inline AbstractTask(const std::function<T()>& fn_) : work(std::move(fn_)) {}

    void operator()() override {
        C* that = ((C*)this);
        that->invoke();
        that->task_complete();
    }

};

/**
 * An asynchronous task. Disposes itself when complete.
 */
template <typename T>
class AsyncTask : public AbstractTask<T, AsyncTask<T>>
{
    using super = AbstractTask<T,AsyncTask<T>>;

public:
    inline AsyncTask(const std::function<T()>& fn_) :  super(fn_) {}

    inline void task_complete()
    {
        delete this;
    }

    inline void invoke()
    {
        this->work();
    }

};

/**
 * Promises. these are used for synchronous tasks.
 */
template<typename T, typename C> class AbstractPromise : public AbstractTask<T,C>
{

    os_semaphore_t complete;

protected:
    using task = AbstractTask<T, C>;

    void wait_complete()
    {
        os_semaphore_take(complete, CONCURRENT_WAIT_FOREVER, false);
    }

public:

    AbstractPromise(const std::function<T()>& fn_) : task(fn_), complete(nullptr)
    {
        os_semaphore_create(&complete, 1, 0);
    }

    virtual ~AbstractPromise()
    {
        dispose();
    }

    inline void dispose()
    {
        if (complete)
        {
            os_semaphore_destroy(complete);
            complete = nullptr;
        }
    }

    inline void task_complete()
    {
        os_semaphore_give(complete, false);
    }

};

/**
 * A promise that executes a function and returns the function result as the future
 * value.
 */
template<typename T> class Promise : public AbstractPromise<T, Promise<T>>
{
    /**
     * The result retrieved from the function.
     * Only valid once {@code complete} is {@code true}
     */
    T result;

    using super = AbstractPromise<T, Promise<T>>;
    friend typename super::task;

    void invoke()
    {
        this->result = this->work();
    }

public:

    Promise(const std::function<T()>& fn_) : super(fn_) {}
    virtual ~Promise() = default;

    /**
     * wait for the result
     */
    T get()
    {
        this->wait_complete();
        return result;
    }
};

/**
 * Specialization of Promise that waits for execution of a function returning void.
 */
template<> class Promise<void> : public AbstractPromise<void, Promise<void>>
{
    using super = AbstractPromise<void, Promise<void>>;
    friend typename super::task;

    inline void invoke()
    {
        this->work();
    }

public:

    Promise(const std::function<void()>& fn_) : super(fn_) {}
    virtual ~Promise() = default;

    void get()
    {
        this->wait_complete();
    }
};

class ISRTaskPool;

/**
 * Simple asynchronous task that can be scheduled from an ISR.
 */
class ISRTask: public Message
{
public:
    typedef void (*Callback)(void*);

    // Invokes a callback and returns this object back to associated pool
    virtual void operator()() override;

private:
    Callback callback;
    void* data;
    ISRTask* next;
    ISRTaskPool* pool;

    friend class ISRTaskPool;
};

/**
 * Preallocated pool of ISRTask objects.
 */
class ISRTaskPool
{
public:
    explicit ISRTaskPool(size_t size);
    ~ISRTaskPool();

    ISRTask* take(ISRTask::Callback callback, void* data = nullptr);
    void release(ISRTask* task);

private:
    ISRTask* tasks; // All tasks
    ISRTask* availTask; // Next unused task
};


class ActiveObjectBase
{
public:
    using Item = Message*;

protected:

    ActiveObjectConfiguration configuration;

    /**
     * The thread that runs this active object.
     */
    std::thread _thread;

    std::thread::id _thread_id;

    std::mutex _start;

    ISRTaskPool isr_tasks;

    volatile bool started;

    /**
     * The main run loop for an active object.
     */
    void run();

protected:


    // todo - concurrent queue should be a strategy so it's pluggable without requiring inheritance
    virtual bool take(Item& item)=0;
    virtual bool put(Item& item)=0;

    void set_thread(std::thread&& thread)
    {
        this->_thread.swap(thread);
        _thread_id = _thread.get_id();
    }

    /**
     * Static thread entrypoint to run this active object loop.
     * @param obj
     */
    static void run_active_object(ActiveObjectBase* obj);

    void start_thread();

public:

    ActiveObjectBase(const ActiveObjectConfiguration& config) :
            configuration(config),
            isr_tasks(config.isr_task_pool_size),
            started(false) {
    }

    bool process();

    bool isCurrentThread() {
        return _thread_id == std::this_thread::get_id();
    }

    void setCurrentThread() {
        _thread_id = std::this_thread::get_id();
    }

    bool isStarted() {
        return started;
    }

    template<typename R> void invoke_async(const std::function<R(void)>& work)
    {
        auto task = new AsyncTask<R>(work);
        if (task)
        {
			Item message = task;
			if (!put(message))
				delete task;
        }
	}

    template<typename R> Promise<R>* invoke_future(const std::function<R(void)>& work)
    {
        auto promise = new Promise<R>(work);
        if (promise)
        {
			Item message = promise;
			if (!put(message))
			{
				delete promise;
				promise = nullptr;
			}
        }
        return promise;
    }

    bool invoke_async_from_isr(void (*callback)(void*), void* data);
};


template <size_t queue_size=50>
class ActiveObjectChannel : public ActiveObjectBase
{
    cpp::channel<Item, queue_size> _channel;

protected:

    virtual bool take(Item& item) override
    {
        return cpp::select().recv_only(_channel, item).try_once();
    }

    virtual bool put(const Item& item) override
    {
        _channel.send(item);
        return true;
    }


public:

    ActiveObjectChannel(ActiveObjectConfiguration& config) : ActiveObjectBase(config) {}

    /**
     * Start the asynchronous processing for this active object.
     */
    void start()
    {
        _channel = cpp::channel<Item*, queue_size>();
        start_thread();
    }

};

class ActiveObjectQueue : public ActiveObjectBase
{
    os_queue_t  queue;

protected:

    virtual bool take(Item& result)
    {
        return !os_queue_take(queue, &result, configuration.take_wait, nullptr);
    }

    virtual bool put(Item& item)
    {
    		return !os_queue_put(queue, &item, configuration.put_wait, nullptr);
    }

    void createQueue(bool isr_tasks_only = false)
    {
        const size_t size = isr_tasks_only ? configuration.isr_task_pool_size :
                std::max(configuration.queue_size, configuration.isr_task_pool_size);
        os_queue_create(&queue, sizeof(Item), size, nullptr);
    }

public:

    ActiveObjectQueue(const ActiveObjectConfiguration& config) : ActiveObjectBase(config), queue(NULL) {}

    void start()
    {
        createQueue();
    }
};


/**
 * An active object that runs the message pump on the calling thread.
 */
class ActiveObjectCurrentThreadQueue : public ActiveObjectQueue
{
public:
    ActiveObjectCurrentThreadQueue(const ActiveObjectConfiguration& config) : ActiveObjectQueue(config) {}

    /**
     * Start the message pump on this thread. This method does not return.
     */
    void start()
    {
        createQueue();
        setCurrentThread();
        run();
    }

    void process()
    {
        ActiveObjectQueue::process();
    }

    void createISRTaskQueue()
    {
        createQueue(true /* isr_tasks_only */);
    }
};


/**
 * An active object that runs the message pump on a new thread using a queue
 * for the message store.
 */
class ActiveObjectThreadQueue : public ActiveObjectQueue
{

public:

    ActiveObjectThreadQueue(const ActiveObjectConfiguration& config) : ActiveObjectQueue(config) {}

    void start()
    {
        createQueue();
        start_thread();
    }

    void process()
    {
        ActiveObjectQueue::process();
    }

    void createISRTaskQueue()
    {
        // FIXME: Looks like we need to refactor active object classes a little
        configuration.take_wait = 0; // Do not wait on ISR-only task queue
        createQueue(true /* isr_tasks_only */);
    }
};



#endif
