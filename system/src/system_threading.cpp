

#include "system_threading.h"
#include <time.h>

#include <string.h>

ActiveObject SystemThread;

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

    void invoke_thread(void* ptr)
    {
        thread::_Impl_base* call = (thread::_Impl_base*)ptr;
        call->_M_run();
    }

    void thread::_M_start_thread(thread::__shared_base_type base)
    {
        os_thread_create(&_M_id._M_thread, "", 0, invoke_thread, base.get(), 600);
    }

}

void ActiveObject::invoke_impl(void* fn, const void* data, size_t len)
{
    // allocate storage for the message
    void* copy = NULL;
    if (data && len) {
        copy = malloc(len);
        memcpy(copy, data, len);
    }


}


void f()
{
    cpp::channel<char> c;
    c.send('c');
    c.recv_ptr();
    char ch;
    c.recv(ch);

}

ActiveObject::ActiveObject()
{
    std::thread t1(f);
    f();

}

