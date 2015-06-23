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

#if PLATFORM_THREADING

#include "active_object.h"
#include "string.h"

void ActiveObjectBase::start_thread()
{
    // prevent the started thread from running until the thread id has been assigned
    // so that calls to isCurrentThread() work correctly
    std::lock_guard<std::mutex> lck (_start);
    set_thread(std::thread(run_active_object, this));
}


void ActiveObjectBase::run()
{
    std::lock_guard<std::mutex> lck (_start);
    started = true;

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
            configuration.background_task();
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

#endif