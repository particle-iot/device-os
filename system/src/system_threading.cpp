

#include "system_threading.h"
#include "system_task.h"
#include <time.h>
#include <string.h>


#if PLATFORM_THREADING
void system_thread_idle()
{
    Spark_Idle_Events(true);
}

ActiveObject SystemThread(ActiveObjectConfiguration(system_thread_idle, 1024*3));


namespace std {
    condition_variable::~condition_variable()
    {
        os_condition_variable_destroy(&_M_cond);
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

    mutex& __get_once_mutex() {
        static mutex __once;
        return __once;
    }

    function<void()> __once_functor;

    __future_base::_Result_base::_Result_base() = default;
    __future_base::_Result_base::~_Result_base() = default;
    //__future_base::_State_base::~_State_base() = default;

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
        if (os_thread_create(&_M_id._M_thread, "", 0, invoke_thread, base.get(), 1024*20)) {
            PANIC(AssertionFailure, "%s %", __FILE__, __LINE__);
        }
    }

    inline std::unique_lock<std::mutex>*& __get_once_functor_lock_ptr()
    {
      static std::unique_lock<std::mutex>* __once_functor_lock_ptr = 0;
      return __once_functor_lock_ptr;
    }

    void __set_once_functor_lock_ptr(unique_lock<mutex>* __ptr)
    {
        __get_once_functor_lock_ptr() = __ptr;
    }
}

#endif