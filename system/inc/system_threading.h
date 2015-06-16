/**
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

#include <stddef.h>
#include "concurrent_hal.h"
#include "channel.h"



class ActiveObject
{
    struct Item
    {
        void* function;
        void* arg;          // the argument is dynamically allocated
    };


    void invoke_impl(void* fn, const void* data, size_t len);

    std::thread _thread;
    cpp::channel<Item> _channel;


public:
    ActiveObject();

    bool isCurrentThread() {
        return _thread.native_handle()==__gthread_self();
    }

    template<typename arg, typename r> inline void invoke(r (*fn)(arg* a), arg* value, size_t size) {
        invoke_impl((void*)fn, value, size);
    }
    void invoke(void (*fn)());
};

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

