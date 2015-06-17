/**7
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

#ifndef SYSTEM_THREADING_H
#define	SYSTEM_THREADING_H

#include "concurrent_hal.h"
#include "channel.h"
#include <stddef.h>
#include <functional>

class ActiveObjectBase
{
public:
    /**
     * Function to call when there are no objects to process in the queue.
     */
    typedef std::function<void(void)> background_task_t;


protected:

    background_task_t background_task;

    void invoke_impl(void* fn, void* data, size_t len);

    /**
     * The thread that runs this active object.
     */
    std::thread _thread;

    /**
     * The main run loop for an active object.
     */
    void run();

protected:

    /**
     * Describes a monadic function to be called by the active object.
     */
    struct Item
    {
        typedef void (*active_fn_t)(void*);

        active_fn_t function;
        void* arg;              // the argument is dynamically allocated

        Item() : function(NULL), arg(NULL){}
        Item(active_fn_t f, void* a) : function(f), arg(a){}

        void invoke()
        {
            if (function) {
                function(arg);
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
    }

    /**
     * Static thread entrypoint to run this active object loop.
     * @param obj
     */
    static void run_active_object(ActiveObjectBase* obj);

    void start_thread() { set_thread(std::thread(run_active_object, this)); }

public:

    ActiveObjectBase(background_task_t task) : background_task(task) {}

    bool isCurrentThread() {
        __gthread_t thread_handle = _thread.native_handle();
        __gthread_t current = __gthread_self();
        return __gthread_equal(thread_handle, current);
    }

    template<typename arg, typename r> inline void invoke(r (*fn)(arg* a), arg* value, size_t size) {
        invoke_impl((void*)fn, value, size);
    }

    void invoke(void (*fn)()) {
        invoke_impl((void*)fn, NULL, 0);
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

    ActiveObjectChannel(background_task_t task) : ActiveObjectBase(task) {}

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
        return os_queue_take(queue, &item, 0);
    }

    virtual void put(const Item& item)
    {
        while (!os_queue_put(queue, &item, CONCURRENT_WAIT_FOREVER)) {}
    }

public:

    ActiveObjectQueue(background_task_t task) : ActiveObjectBase(task), queue(NULL) {}

    void start()
    {
        os_queue_create(&queue, sizeof(Item), 50);
        start_thread();
    }
};

typedef ActiveObjectQueue ActiveObject;

extern ActiveObject SystemThread;
extern ActiveObject AppThread;


// execute the enclosing void function async on the system thread
#define SYSTEM_THREAD_CONTEXT_RESULT(result) \
    if (SystemThread.isCurrentThread()) {\
        SYSTEM_THREAD_CONTEXT_FN0(__func__); \
        return result; \
    }

#define SYSTEM_THREAD_CONTEXT(result)


#define SYSTEM_THREAD_CONTEXT_FN0(fn) \
    SystemThread.invoke(fn);


#define SYSTEM_THREAD_CONTEXT_FN1(fn, arg, sz) \
    SystemThread.invoke(fn, arg, sz)

// execute synchornously on the system thread
#define SYSTEM_THREAD_CONTEXT_SYNC()



#endif	/* SYSTEM_THREADING_H */

