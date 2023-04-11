#include "system_threading.h"
#include "system_task.h"
#include <time.h>
#include <string.h>
#include "hal_platform.h"

#if PLATFORM_THREADING

namespace particle {

namespace {

os_mutex_recursive_t usb_serial_mutex;

void system_thread_idle()
{
    Spark_Idle_Events(true);
}

} // namespace

ActiveObjectThreadQueue SystemThread(ActiveObjectConfiguration(system_thread_idle,
			100, /* take timeout */
			0x7FFFFFFF, /* put timeout - wait forever */
			50, /* queue size */
			HAL_PLATFORM_SYSTEM_THREAD_STACK_SIZE /* stack size */,
            OS_THREAD_PRIORITY_DEFAULT, /* default priority */
            HAL_PLATFORM_SYSTEM_THREAD_TASK_NAME /* task name */));

os_mutex_recursive_t mutex_usb_serial()
{
    if (nullptr==usb_serial_mutex) {
        os_mutex_recursive_create(&usb_serial_mutex);
    }
    return usb_serial_mutex;
}

} // namespace particle

/**
 * Implementation to support gthread's concurrency primitives.
 */
namespace std {

#if 0
	/**
	 * Imlementation of conditional varaible in terms of the HAL. This has
	 * a potential race condition due to the placement of the critical section,
	 * so has been commented out. https://github.com/spark/firmware/pull/614#discussion-diff-39882530
	 */
    condition_variable::~condition_variable()
    {
        os_condition_variable_destroy(_M_cond);
    }

    condition_variable::condition_variable()
    {
        os_condition_variable_create(&_M_cond);
    }

    void condition_variable::wait(unique_lock<mutex>& lock)
    {
        os_condition_variable_wait(_M_cond, &lock);
    }

    void condition_variable::notify_one()
    {
        os_condition_variable_notify_one(_M_cond);
    }

    void condition_variable::notify_all()
    {
        os_condition_variable_notify_all(_M_cond);
    }

    __future_base::_Result_base::_Result_base() = default;
    __future_base::_Result_base::~_Result_base() = default;

     #if __GNUC__ == 4 && __GNUC_MINOR__ == 8
    __future_base::_State_base::~_State_base() = default;
    #endif

    struct thread_startup
    {
    	thread::_Impl_base* call;
    	volatile bool started;
    };

    /**
     * static Startup function for threads.
     * @param ptr   A pointer to the _Impl_base value which exposes the virtual
     *  run() method.
     */
    void invoke_thread(void* ptr)
    {
        thread_startup* startup = (thread_startup*)ptr;
        thread::__shared_base_type local(startup->call);
        thread::_Impl_base* call = (thread::_Impl_base*)local.get();
        startup->started = true;
        call->_M_run();
    }

    void thread::_M_start_thread(thread::__shared_base_type base)
    {
        thread_startup startup;
        startup.call = base.get();
        startup.started = false;
        if (os_thread_create(&_M_id._M_thread, "std::thread", OS_THREAD_PRIORITY_DEFAULT, invoke_thread, &startup, THREAD_STACK_SIZE)) {
            PANIC(AssertionFailure, "%s %s", __FILE__, __LINE__);
        }
        else {  // C++ ensure the thread has started execution, as required by the standard
            while (!startup.started) os_thread_yield();
        }
    }
#endif // 0

    function<void()> __once_functor;

    mutex& __get_once_mutex() {
        static mutex __once;
        return __once;
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

#endif // PLATFORM_THREADING
