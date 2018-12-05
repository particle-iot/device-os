/**
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

#include "active_object.h"

#include "spark_wiring_interrupts.h"
#include "debug.h"

#if PLATFORM_THREADING

#include <string.h>
#include "concurrent_hal.h"
#include "timer_hal.h"

void ActiveObjectBase::start_thread()
{
    // prevent the started thread from running until the thread id has been assigned
    // so that calls to isCurrentThread() work correctly
    set_thread(std::thread(run_active_object, this));
    while (!started) {
        os_thread_yield();
    }
}


void ActiveObjectBase::run()
{
    /* XXX: We shouldn't constantly hold a mutex. This breaks priority inhertiance mechanisms in FreeRTOS. */
    /* It's not even used anywhere */
    // std::lock_guard<std::mutex> lck (_start);
    started = true;

    uint32_t last_background_run = 0;
    for (;;)
    {
    	uint32_t now;
        if (!process())
		{
        	configuration.background_task();
        }
        else if ((now=HAL_Timer_Get_Milli_Seconds())-last_background_run > configuration.take_wait)
        {
        	last_background_run = now;
        	configuration.background_task();
        }
    }
}

bool ActiveObjectBase::process()
{
    bool result = false;
    Item item = nullptr;
    if (take(item) && item)
    {
        Message& msg = *item;
        msg();
        result = true;
    }
    return result;
}

void ActiveObjectBase::run_active_object(ActiveObjectBase* object)
{
    object->run();
}

#endif // PLATFORM_THREADING

void ISRTaskQueue::enqueue(Task* task) {
    ATOMIC_BLOCK() {
        // Add task object to the queue
        if (lastTask_) {
            lastTask_->next = task;
        } else { // The queue is empty
            firstTask_ = task;
        }
        task->next = nullptr;
        lastTask_ = task;
    }
}

bool ISRTaskQueue::process() {
    Task* task = nullptr;
    if (!firstTask_) {
        return false;
    }
    ATOMIC_BLOCK() {
        // Take task object from the queue
        task = firstTask_;
        firstTask_ = task->next;
        if (!firstTask_) {
            lastTask_ = nullptr;
        }
    }
    // Invoke task function
    task->func(task);
    return true;
}
