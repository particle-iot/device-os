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

#include "spark_wiring_thread.h"
#include "delay_hal.h"
#include "timer_hal.h"
#include "system_task.h"

#if PLATFORM_THREADING
#include "system_monitor.h"

class ApplicationWatchdog
{
	volatile system_tick_t timeout;
	static volatile system_tick_t last_checkin;

	std::function<void(void)> timeout_fn;

	Thread thread;

	static void start(void* pointer);

	void loop();

public:
    static const unsigned DEFAULT_STACK_SIZE = 512;

	ApplicationWatchdog(unsigned timeout_ms, std::function<void(void)> fn, unsigned stack_size=DEFAULT_STACK_SIZE) :
		timeout(timeout_ms),
		timeout_fn(fn),
		thread("appwdt", start, this, OS_THREAD_PRIORITY_CRITICAL, stack_size)
	{
		checkin();
	}

    // This constuctor helps to resolve overloaded function types, such as System.reset(), which is not always
    // possible in case of std::function
    ApplicationWatchdog(unsigned timeout_ms, void (*fn)(), unsigned stack_size=DEFAULT_STACK_SIZE) :
        ApplicationWatchdog(timeout_ms, std::function<void()>(fn), stack_size)
    {
    }

	bool isComplete()
	{
		return !timeout_fn;
	}

	static inline system_tick_t current_time()
	{
		return HAL_Timer_Get_Milli_Seconds();
	}

	bool has_timedout()
	{
		return (current_time()-last_checkin)>=timeout;
	}

	/**
	 * Dispose of this thread next time it wakes up.
	 */
	void dispose()
	{
		timeout = 0;
	}

	/**
	 * Lifesign that the application is still working normally.
	 */
	static void checkin()
	{
		last_checkin = current_time();
	}

};

namespace spark {

const system_tick_t DEFAULT_WATCHDOG_TIMEOUT = 1000;

enum WatchdogFeature {
    WATCHDOG_FEATURE_IWDG = 0x01
};

class WatchdogClass {
public:
    // Enables monitoring for the calling thread with a specified period
    static bool enable(system_tick_t timeout = DEFAULT_WATCHDOG_TIMEOUT) {
        return system_monitor_enable(os_thread_current(), timeout, nullptr) == 0;
    }
    // Disable monitoring for the calling thread
    static bool disable() {
        return system_monitor_disable(os_thread_current(), nullptr) == 0;
    }

    // Reports that the calling thread is alive
    static bool kick() {
        return system_monitor_kick(os_thread_current(), nullptr) == 0;
    }

    // Gets the timeout for the calling thread. Returns 0 if monitoring is not
    // enabled for the calling thread.
    static system_tick_t getTimeout() {
        return getTimeout(os_thread_current());
    }
    // Gets the timeout for a specified thread. Returns 0 if monitoring is not
    // enabled for the specified thread.
    static system_tick_t getTimeout(os_thread_t thread) {
        return system_monitor_get_thread_timeout(thread, nullptr);
    }
    // Changes the timeout for the calling thread. Returns false if monitoring is not
    // enabled for the calling thread.
    static bool setTimeout(system_tick_t timeout) {
        return setTimeout(os_thread_current(), timeout);
    }
    // Changes the timeout for the specified thread. Returns false if monitoring is not
    // enabled for the specified thread.
    static bool setTimeout(os_thread_t thread, system_tick_t timeout) {
        return system_monitor_set_thread_timeout(thread, timeout, nullptr) == 0;
    }

    // Returns maximum amount of time the device may sleep, or 0 if indefinitely
    static system_tick_t getMaximumSleepTime() {
        return system_monitor_get_max_sleep_time(nullptr, nullptr);
    }

    // There should be the same System.enableFeature() overload for convenience
    // WatchdogFeature:
    // - WATCHDOG_FEATURE_IWDG - enables IWDG (independent watchdog)
    static bool enableFeature(WatchdogFeature feature) {
        system_monitor_configuration_t config = {};
        config.size = sizeof(config);
        // config.version = 0;
        if (feature == WATCHDOG_FEATURE_IWDG) {
            config.iwdg = 1;
        }
        return system_monitor_configure(&config, nullptr) == 0;
    }
};

} /* namespace spark */

extern spark::WatchdogClass Watchdog;

inline void application_checkin() {
    SPARK_ASSERT(application_thread_current(nullptr));
    spark::WatchdogClass::kick();
    ApplicationWatchdog::checkin();
}

#else

inline void application_checkin() {  }

#endif

