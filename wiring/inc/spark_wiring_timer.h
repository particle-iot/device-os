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

#pragma once

#if PLATFORM_ID!=3
#include "stddef.h"
#include "concurrent_hal.h"

class Timer
{
public:

    typedef void (*timer_callback_fn)(Timer& timer);

    Timer(unsigned period, void (*callback)()) : Timer(period, timer_callback_fn(callback)) {}

    Timer(unsigned period, timer_callback_fn callback_) : handle(nullptr), callback(callback_) {
        os_timer_create(&handle, period, invoke_timer, this, nullptr);
    }


    virtual ~Timer() { dispose(); }

    void startFromISR() { start(true); }
    void stopFromISR() { stop(true); }
    void resetFromISR() { reset(true); }


    void start(bool fromISR=false)
    {
        stop(fromISR);
        if (handle)
            os_timer_change(handle, OS_TIMER_CHANGE_START, 0, 0, fromISR, nullptr);
    }

    void stop(bool fromISR=false)
    {
        if (handle)
            os_timer_change(handle, OS_TIMER_CHANGE_STOP, 0, 0, fromISR, nullptr);
    }

    void reset(bool fromISR=false)
    {
        if (handle)
            os_timer_change(handle, OS_TIMER_CHANGE_RESET, 0, 0, fromISR, nullptr);
    }

    void dispose()
    {
        if (handle) {
            os_timer_destroy(handle, nullptr);
            handle = nullptr;
        }
    }

    /*
     * Subclasses can either provide a callback function, or override
     * this timeout method.
     */
    virtual void timeout()
    {
        if (callback)
            callback(*this);
    }

private:

    os_timer_t handle;
    timer_callback_fn callback;

    static void invoke_timer(os_timer_t timer)
    {
        void* timer_id = NULL;
        if (!os_timer_get_id(timer, &timer_id)) {
            if (timer_id)
                ((Timer*)timer_id)->timeout();
        }
    }


};

#endif