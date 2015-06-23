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

#include <functional>

#if PLATFORM_THREADING
#include "concurrent_hal.h"
#include "active_object.h"
#include <stddef.h>
#include <mutex>
#include <future>

#ifndef PARTICLE_GTHREAD_INCLUDED
#error "GTHREAD header not included. This is required for correct mutex implementation on embedded platforms."
#endif

typedef ActiveObjectQueue ActiveObject;

extern ActiveObject SystemThread;
extern ActiveObject AppThread;

#endif

// execute the enclosing void function async on the system thread
#if PLATFORM_THREADING
#define SYSTEM_THREAD_CONTEXT_RESULT(result) \
    if (SystemThread.isStarted() && !SystemThread.isCurrentThread()) {\
        SYSTEM_THREAD_CONTEXT_FN0(__func__); \
        return result; \
    }
#else
#define SYSTEM_THREAD_CONTEXT_RESULT(result)
#endif


#if PLATFORM_THREADING
#define SYSTEM_THREAD_CONTEXT_FN0(fn) \
    SystemThread.invoke(fn);
#else
    #define SYSTEM_THREAD_CONTEXT_FN0(fn)
#endif

#if PLATFORM_THREADING
#define SYSTEM_THREAD_CONTEXT_FN1(fn, arg, sz) \
    SystemThread.invoke(fn, arg, sz)
#else
#define SYSTEM_THREAD_CONTEXT_FN1(fn, arg, sz)
#endif


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


// execute synchrnously on the system thread. Since the parameter lifetime is
// assumed to be bound by the caller, the parameters don't need marshalling
// fn: the function call to perform. This is textually substitued into a lambda, with the
// parameters passed by copy.
#if PLATFORM_THREADING
#define SYSTEM_THREAD_CONTEXT_SYNC(fn) \
    if (SystemThread.isStarted() && !SystemThread.isCurrentThread()) { \
        auto lambda = [=]() { return (fn); }; \
        auto future = SystemThread.invoke_future(FFL(lambda)); \
        future.wait(); \
        return future.get(); \
    }
#else
#define SYSTEM_THREAD_CONTEXT_SYNC(fn)
#endif

#if PLATFORM_THREADING
#define SYSTEM_THREAD_START()  SystemThread.start()
#define SYSTEM_THREAD_CURRENT() (SystemThread.isCurrentThread())
#else
#define SYSTEM_THREAD_START()
#define SYSTEM_THREAD_CURRENT() (1)
#endif


#endif	/* SYSTEM_THREADING_H */

