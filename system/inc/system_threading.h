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

#include "active_object.h"
#include "system_error.h"

#include <utility>
#include <new>

#if PLATFORM_THREADING

#include "concurrent_hal.h"
#include <stddef.h>
#include <functional>
#include <mutex>
#include <future>

#if !defined(PARTICLE_GTHREAD_INCLUDED)
#error "GTHREAD header not included. This is required for correct mutex implementation on embedded platforms."
#endif

namespace particle {

/**
 * System thread runs on a separate thread
 */
extern ActiveObjectThreadQueue SystemThread;

/**
 * Application queue runs on the calling thread (main)
 */
extern ActiveObjectCurrentThreadQueue ApplicationThread;

template<typename T>
struct memfun_type
{
    using type = void;
};

template<typename Ret, typename Class, typename... Args>
struct memfun_type<Ret(Class::*)(Args...) const>
{
    using type = std::function<Ret(Args...)>;
};

template<typename F>
typename memfun_type<decltype(&F::operator())>::type
FFL(F const &func)
{ // Function from lambda !
    return func;
}

os_mutex_recursive_t mutex_usb_serial();

} // namespace particle

#define _THREAD_CONTEXT_ASYNC_RESULT(thread, fn, result) \
    if (thread.isStarted() && !thread.isCurrentThread()) { \
        auto lambda = [=]() { (fn); }; \
        thread.invoke_async(particle::FFL(lambda)); \
        return result; \
    }

#define _THREAD_CONTEXT_ASYNC(thread, fn) \
    if (thread.isStarted() && !thread.isCurrentThread()) { \
        auto lambda = [=]() { (fn); }; \
        thread.invoke_async(particle::FFL(lambda)); \
        return; \
    }

// execute synchronously on the system thread. Since the parameter lifetime is
// assumed to be bound by the caller, the parameters don't need marshalling
// fn: the function call to perform. This is textually substitued into a lambda, with the
// parameters passed by copy.
#define SYSTEM_THREAD_CONTEXT_SYNC(fn) \
    if (particle::SystemThread.isStarted() && !particle::SystemThread.isCurrentThread()) { \
        auto callable = particle::FFL([=]() { return (fn); }); \
        auto future = particle::SystemThread.invoke_future(callable); \
        auto result = future ? future->get() : 0;  \
        delete future; \
        return result; \
    }

#define SYSTEM_THREAD_CURRENT() (particle::SystemThread.isCurrentThread())
#define APPLICATION_THREAD_CURRENT() (particle::ApplicationThread.isCurrentThread())

#else // !PLATFORM_THREADING

#define _THREAD_CONTEXT_ASYNC(thread, fn)
#define _THREAD_CONTEXT_ASYNC_RESULT(thread, fn, result)
#define SYSTEM_THREAD_CONTEXT_SYNC(fn)

#define SYSTEM_THREAD_CURRENT() (1)
#define APPLICATION_THREAD_CURRENT() (1)

#endif // !PLATFORM_THREADING

#define SYSTEM_THREAD_CONTEXT_ASYNC(fn) _THREAD_CONTEXT_ASYNC(particle::SystemThread, fn)
#define SYSTEM_THREAD_CONTEXT_ASYNC_RESULT(fn, result) _THREAD_CONTEXT_ASYNC_RESULT(particle::SystemThread, fn, result)
#define APPLICATION_THREAD_CONTEXT_ASYNC(fn) _THREAD_CONTEXT_ASYNC(particle::ApplicationThread, fn)
#define APPLICATION_THREAD_CONTEXT_ASYNC_RESULT(fn, result) _THREAD_CONTEXT_ASYNC_RESULT(particle::ApplicationThread, fn, result)

// Perform an asynchronous function call if not on the system thread,
// or execute directly if on the system thread
#define SYSTEM_THREAD_CONTEXT_ASYNC_CALL(fn) \
    SYSTEM_THREAD_CONTEXT_ASYNC(fn); \
    fn;

#define SYSTEM_THREAD_CONTEXT_SYNC_CALL(fn) \
    SYSTEM_THREAD_CONTEXT_SYNC(fn); \
    fn;

#define SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(fn) \
    SYSTEM_THREAD_CONTEXT_SYNC(fn); \
    return fn;

namespace particle {

namespace detail {

struct CallableTaskBase: ISRTaskQueue::Task {
    virtual void call() = 0;
};

template<typename F>
struct CallableTask: CallableTaskBase {
    F fn;

    explicit CallableTask(F&& fn) : fn(std::move(fn)) {
    }

    void call() override {
        fn();
    }
};

} // namespace detail

extern ISRTaskQueue SystemISRTaskQueue;

/**
 * Asynchronously invokes a function in the context of the system thread via the ISR task queue.
 *
 * @note This function allocates memory and thus it cannot be called from an ISR.
 */
template<typename F>
int invokeAsync(F&& fn) {
    // Not using std::function here as it's not exception-safe
    auto task = new(std::nothrow) detail::CallableTask<F>(std::move(fn));
    if (!task) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    task->func = [](ISRTaskQueue::Task* task) {
        static_cast<detail::CallableTaskBase*>(task)->call();
        delete task;
    };
    SystemISRTaskQueue.enqueue(task);
    return 0;
}

} // namespace particle

#endif	/* SYSTEM_THREADING_H */
