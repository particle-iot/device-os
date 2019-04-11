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
#include "spark_wiring_thread.h"
#include <functional>

class Timer
{
public:

    typedef std::function<void(void)> timer_callback_fn;

    Timer(unsigned period, timer_callback_fn callback_, bool one_shot=false) : running(false), handle(nullptr), callback(std::move(callback_)) {
        os_timer_create(&handle, period, invoke_timer, this, one_shot, nullptr);
    }

    template <typename T>
    Timer(unsigned period, void (T::*handler)(), T& instance, bool one_shot=false) : Timer(period, std::bind(handler, &instance), one_shot)
    {
    }

    virtual ~Timer() {
    		// when the timer is calling the std::function, we cannot dispose of it until the function completes.
		// the call has exited.
		dispose();
    }

    bool startFromISR() { return _start(0, true); }
    bool stopFromISR() { return _stop(0, true); }
    bool resetFromISR() { return _reset(0, true); }
    bool changePeriodFromISR(unsigned period) { return _changePeriod(period, 0, true); }

    static const unsigned default_wait = 0x7FFFFFFF;

    bool start(unsigned block=default_wait) { return _start(block, false); }
    bool stop(unsigned block=default_wait) { return _stop(block, false); }
    bool reset(unsigned block=default_wait) { return _reset(block, false); }
    bool changePeriod(unsigned period, unsigned block=default_wait) { return _changePeriod(period, block, false); }

    bool isValid() const { return handle!=nullptr; }
    bool isActive() const { return isValid() && os_timer_is_active(handle, nullptr); }

    bool _start(unsigned block, bool fromISR=false)
    {
        stop(fromISR);
        return handle ? !os_timer_change(handle, OS_TIMER_CHANGE_START, fromISR, 0, block, nullptr) : false;
    }

    bool _stop(unsigned block, bool fromISR=false)
    {
        return handle ? !os_timer_change(handle, OS_TIMER_CHANGE_STOP, fromISR, 0, block, nullptr) : false;
    }

    bool _reset(unsigned block, bool fromISR=false)
    {
        return handle ? !os_timer_change(handle, OS_TIMER_CHANGE_RESET, fromISR, 0, block, nullptr) : false;
    }

    bool _changePeriod(unsigned period, unsigned block, bool fromISR=false)
    {
         return handle ? !os_timer_change(handle, OS_TIMER_CHANGE_PERIOD, fromISR, period, block, nullptr) : false;
    }

    void dispose()
    {
        if (handle) {
            stop();
            // Make sure the callback will not be called after this object is destroyed.
            // TODO: Consider assigning a higher priority to the timer thread
            os_timer_set_id(handle, nullptr);
            while (running) {
                os_thread_yield();
            }
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
        if (callback) {
            callback();
        }
    }

private:
	volatile bool running;
    os_timer_t handle;
    timer_callback_fn callback;

    static void invoke_timer(os_timer_t timer)
    {
        Timer* t = nullptr;
        SINGLE_THREADED_BLOCK() {
            void* id = nullptr;
            os_timer_get_id(timer, &id);
            t = static_cast<Timer*>(id);
            if (t) {
                t->running = true;
            }
        }
        if (t) {
            t->timeout();
            t->running = false;
        }
    }

};

#endif // PLATFORM_ID!=3

#if (0 == PLATFORM_THREADING)
namespace particle
{
class NullTimer
{
public:
    typedef std::function<void(void)> timer_callback_fn;

    NullTimer(unsigned, timer_callback_fn, bool)
    {
    }

    template <typename T>
    NullTimer(unsigned, void (T::*)(), T&, bool)
    {
    }

    inline static bool changePeriod(const size_t)
    {
        return true;
    }
    inline static void dispose(void)
    {
    }
    inline static bool isActive(void)
    {
        return false;
    }
    inline static void reset(void)
    {
    }
    inline static void start(void)
    {
    }
    inline static void stop(void)
    {
    }
};
} // namespace particle
#endif // not PLATFORM_THREADING
