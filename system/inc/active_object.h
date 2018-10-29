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

#include <cstddef>

#if PLATFORM_THREADING

#include <functional>
#include <mutex>
#include <thread>
#include <future>

#include "channel.h"
#include "concurrent_hal.h"

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

public:
    ActiveObjectConfiguration(background_task_t task, unsigned take_wait_, unsigned put_wait_,
    			uint16_t queue_size_,
            size_t stack_size_ =0) : background_task(task), stack_size(stack_size_),
            take_wait(take_wait_), put_wait(put_wait_), queue_size(queue_size_) {}

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
template<typename T> class SystemPromise : public AbstractPromise<T, SystemPromise<T>>
{
    /**
     * The result retrieved from the function.
     * Only valid once {@code complete} is {@code true}
     */
    T result;

    using super = AbstractPromise<T, SystemPromise<T>>;
    friend typename super::task;

    void invoke()
    {
        this->result = this->work();
    }

public:

    SystemPromise(const std::function<T()>& fn_) : super(fn_) {}
    virtual ~SystemPromise() = default;

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
 * Specialization of SystemPromise that waits for execution of a function returning void.
 */
template<> class SystemPromise<void> : public AbstractPromise<void, SystemPromise<void>>
{
    using super = AbstractPromise<void, SystemPromise<void>>;
    friend typename super::task;

    inline void invoke()
    {
        this->work();
    }

public:

    SystemPromise(const std::function<void()>& fn_) : super(fn_) {}
    virtual ~SystemPromise() = default;

    void get()
    {
        this->wait_complete();
    }
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

    ActiveObjectBase(const ActiveObjectConfiguration& config) : configuration(config), started(false) {}

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

    template<typename R> SystemPromise<R>* invoke_future(const std::function<R(void)>& work)
    {
        auto promise = new SystemPromise<R>(work);
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

    void createQueue()
    {
        os_queue_create(&queue, sizeof(Item), configuration.queue_size, nullptr);
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

};



#endif // PLATFORM_THREADING

/**
 * This class implements a queue of asynchronous calls that can be scheduled from an ISR and then
 * invoked from an event loop running in a regular thread.
 */
class ISRTaskQueue {
public:
    struct Task;
    typedef void(*TaskFunc)(Task*);

    struct Task {
        TaskFunc func;
        Task* next; // Next element in the queue
    };

    ISRTaskQueue() :
            firstTask_(nullptr),
            lastTask_(nullptr) {
    }

    void enqueue(Task* task);
    bool process();

private:
    Task* volatile firstTask_;
    Task* lastTask_;
};
