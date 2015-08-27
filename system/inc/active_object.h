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
     * Time to wait for a message in the queue.
     */
    unsigned take_wait;


public:
    ActiveObjectConfiguration(background_task_t task, unsigned take_wait_, size_t stack_size_ =0) : background_task(task), stack_size(stack_size_), take_wait(take_wait_) {}

};


class ActiveObjectBase
{
public:

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


    void invoke_impl(void* fn, void* data, size_t len=0);

protected:

    /**
     * Describes a monadic function to be called by the active object.
     */
    struct Item
    {
        typedef void (*active_fn_t)(void*);
        typedef std::packaged_task<void()> task_t;

        active_fn_t function;
        void* arg;              // the argument is dynamically allocated

        task_t task;

        Item() : function(NULL), arg(NULL){}
        Item(active_fn_t f, void* a) : function(f), arg(a){}
        Item(task_t&& _task) : function(NULL), arg(NULL), task(std::move(_task)) {}

        void invoke()
        {
            if (function) {
                function(arg);
            }
            else {
                task();
            }
        }

        void dispose()
        {
            free(arg);
            function = NULL;
            arg = NULL;
        }
    };

    // todo - concurrent queue should be a strategy so it's pluggable without requiring inheritance
    virtual bool take(Item& item)=0;
    virtual void put(const Item& item)=0;

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

    bool isCurrentThread() {
        return _thread_id == std::this_thread::get_id();
    }

    bool isStarted() {
        return started;
    }

    template<typename arg, typename r> inline void invoke(r (*fn)(arg* a), arg* value, size_t size) {
        invoke_impl((void*)fn, value, size);
    }

    template<typename arg, typename r> inline void invoke(r (*fn)(arg* a), arg* value) {
        invoke_impl((void*)fn, value);
    }

    void invoke(void (*fn)()) {
        invoke_impl((void*)fn, NULL, 0);
    }

    template<typename R> std::future<R> invoke_future(std::function<R(void)> fn) {
        auto task = std::make_shared<std::packaged_task<R()>>(fn);
        //put(Item(std::packaged_task<void()>([=]{ (*task)(); })));
        return (*task).get_future();
    }

};


template <size_t queue_size=50>
class ActiveObjectChannel : public ActiveObjectBase
{
    cpp::channel<Item, queue_size> _channel;

protected:

    virtual bool take(Item& item)
    {
        return cpp::select().recv_only(_channel, item).try_once();
    }

    virtual void put(const Item& item)
    {
        _channel.send(item);
    }


public:

    ActiveObjectChannel(ActiveObjectConfiguration& config) : ActiveObjectBase(config) {}

    /**
     * Start the asynchronous processing for this active object.
     */
    void start()
    {
        _channel = cpp::channel<Item, queue_size>();
        start_thread();
    }

};

class ActiveObjectQueue : public ActiveObjectBase
{
    os_queue_t  queue;

protected:

    virtual bool take(Item& item)
    {
        return os_queue_take(queue, &item, configuration.take_wait)==0;
    }

    virtual void put(const Item& item)
    {
        while (!os_queue_put(queue, &item, CONCURRENT_WAIT_FOREVER)) {}
    }

    void createQueue(int size=50)
    {
        os_queue_create(&queue, sizeof(Item), size);
    }

public:

    ActiveObjectQueue(const ActiveObjectConfiguration& config) : ActiveObjectBase(config), queue(NULL) {}

    void start()
    {
        createQueue();
    }
};


class ActiveObjectCurrentThreadQueue : ActiveObjectQueue
{
public:
    ActiveObjectCurrentThreadQueue(const ActiveObjectConfiguration& config) : ActiveObjectQueue(config) {}

    void start()
    {
        createQueue();
        _thread_id = std::this_thread::get_id();
        run();
    }
};

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



#endif