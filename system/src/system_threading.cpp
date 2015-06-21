

#include "system_threading.h"
#include "system_task.h"
#include <time.h>
#include <string.h>

void system_thread_idle()
{
    Spark_Idle_Events(true);
}

ActiveObject SystemThread(system_thread_idle);

namespace std {
    condition_variable::~condition_variable()
    {
        os_condition_variable_dispose(&_M_cond);
    }

    condition_variable::condition_variable()
    {
        os_condition_variable_create(&_M_cond);
    }

    void condition_variable::wait(unique_lock<mutex>& lock)
    {
        os_condition_variable_wait(&_M_cond, &lock);
    }

    void condition_variable::notify_one()
    {
        os_condition_variable_notify_one(&_M_cond);
    }

    void condition_variable::notify_all()
    {
        os_condition_variable_notify_all(&_M_cond);
    }


    /**
     * static Startup function for threads.
     * @param ptr   A pointer to the _Impl_base value which exposes the virtual
     *  run() method.
     */
    void invoke_thread(void* ptr)
    {
        thread::_Impl_base* call = (thread::_Impl_base*)ptr;
        call->_M_run();
    }

    void thread::_M_start_thread(thread::__shared_base_type base)
    {
        if (os_thread_create(&_M_id._M_thread, "", 0, invoke_thread, base.get(), 2048)) {
            PANIC(AssertionFailure, "%s %", __FILE__, __LINE__);
        }
    }
}

void ActiveObjectBase::run()
{
    Item item;
    for (;;)
    {
        if (take(item))
        {
            item.invoke();
            item.dispose();
        }
        else
        {
            background_task();
        }
    }
}

void ActiveObjectBase::invoke_impl(void* fn, void* data, size_t len)
{
    if (isCurrentThread()) {        // run synchronously since we are already on the thread
        Item(Item::active_fn_t(fn), data).invoke();
    }
    else {
        // allocate storage for the message
        void* copy = data;
        if (data && len) {
            copy = malloc(len);
            memcpy(copy, data, len);
        }
        put(Item(Item::active_fn_t(fn), copy));
    }
}

void ActiveObjectBase::run_active_object(ActiveObjectBase* object)
{
    object->run();
}