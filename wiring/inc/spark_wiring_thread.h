/**
 ******************************************************************************
 * @file    spark_wiring_thread.h
 * @authors Matthew McGowan
 * @date    03 March 2015
 ******************************************************************************
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

#ifndef SPARK_WIRING_THREAD_H
#define	SPARK_WIRING_THREAD_H

#if PLATFORM_THREADING

#include "concurrent_hal.h"
#include <stddef.h>

class thread
{
private:    
    mutable os_thread_t handle;
    
    friend class ThreadFactory;
    
    thread(os_thread_t handle_) 
        : handle(handle_)
    {
    }
    
public:    
    
    thread() : handle(OS_THREAD_INVALID_HANDLE) {}
    
    void dispose()
    {
        os_thread_cleanup(handle);
        handle = OS_THREAD_INVALID_HANDLE;
    }
        
    bool join() 
    {
        return os_thread_join(handle)==0;
    }
    
    bool is_valid() 
    {
        return handle!=OS_THREAD_INVALID_HANDLE;
    }
    
    bool is_current()
    {
        return os_thread_is_current(handle);
    }

    thread& operator = (const thread& rhs)
    {
	if (this != &rhs)
        {
            this->handle = rhs.handle;
            rhs.handle = NULL;
        }
	return *this;
    }
};

class ThreadFactory
{
public:    
    thread create(const char* name, os_thread_fn_t function, void* function_param=NULL, 
        os_thread_prio_t priority=OS_THREAD_PRIORITY_DEFAULT, size_t stack_size=OS_THREAD_STACK_SIZE_DEFAULT, void* stack=NULL)
    {
        os_thread_t handle;
        if (stack)
            os_thread_create_with_stack(&handle, name, priority, function, function_param, stack_size, stack);
        else
            os_thread_create(&handle, name, priority, function, function_param, stack_size);
        return thread(handle);
    }
        
};

extern ThreadFactory Thread;

#endif

#endif	/* SPARK_WIRING_THREAD_H */

