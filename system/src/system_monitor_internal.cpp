/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "system_monitor_internal.h"
#include "platform_config.h"
#include "spark_wiring_interrupts.h"
#include "system_error.h"
#include "timer_hal.h"
#include "core_hal.h"
#include "debug.h"

#if PLATFORM_THREADING

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif // MAX

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif // MIN


namespace {
// 100ms
const system_tick_t WATCHDOG_DEFAULT_TIMEOUT_MS = 100;
// 60ms
const system_tick_t SYSTEM_MONITOR_DEFAULT_TICK_RATE_MS = 60;
// 1 second
const system_tick_t WATCHDOG_INDEPENDENT_TIMEOUT_MS = 1000;

const system_tick_t SYSTEM_MONITOR_DEFAULT_THREAD_TIMEOUT_MS = 1000;

} /* anonymous namespace */

#define HAL_WATCHDOG_CAPABILITY_NONE             (0x00)
// Watchdog timer will reset the CPU
#define HAL_WATCHDOG_CAPABILITY_CPU_RESET        (0x01)
// Watchdog is clocked from an independent clock source
#define HAL_WATCHDOG_CAPABILITY_INDEPENDENT      (0x02)
// Watchdog counter needs to be reset within a configured window
#define HAL_WATCHDOG_CAPABILITY_WINDOWED         (0x04)
// Watchdog timer can be reconfigured
#define HAL_WATCHDOG_CAPABILITY_RECONFIGURABLE   (0x08)
// Watchdog timer can somehow notify about its expiration
#define HAL_WATCHDOG_CAPABILITY_NOTIFY           (0x10)
// Watchdog timer can be stopped
#define HAL_WATCHDOG_CAPABILITY_STOPPABLE        (0x20)

#ifdef DEBUG_BUILD
# ifdef PLATFORM_WATCHDOG_DEFAULT
#  pragma message "Default watchdog: enabled"
#  if HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_DEFAULT) & HAL_WATCHDOG_CAPABILITY_CPU_RESET
#   pragma message "HAL_WATCHDOG_CAPABILITY_CPU_RESET"
#  endif /* HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_DEFAULT) & HAL_WATCHDOG_CAPABILITY_CPU_RESET */
#  if HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_DEFAULT) & HAL_WATCHDOG_CAPABILITY_INDEPENDENT
#   pragma message "HAL_WATCHDOG_CAPABILITY_INDEPENDENT"
#  endif /* HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_DEFAULT) & HAL_WATCHDOG_CAPABILITY_INDEPENDENT */
#  if HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_DEFAULT) & HAL_WATCHDOG_CAPABILITY_WINDOWED
#   pragma message "HAL_WATCHDOG_CAPABILITY_WINDOWED"
#  endif /* HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_DEFAULT) & HAL_WATCHDOG_CAPABILITY_WINDOWED */
#  if HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_DEFAULT) & HAL_WATCHDOG_CAPABILITY_RECONFIGURABLE
#   pragma message "HAL_WATCHDOG_CAPABILITY_RECONFIGURABLE"
#  endif /* HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_DEFAULT) & HAL_WATCHDOG_CAPABILITY_RECONFIGURABLE */
#  if HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_DEFAULT) & HAL_WATCHDOG_CAPABILITY_NOTIFY
#   pragma message "HAL_WATCHDOG_CAPABILITY_NOTIFY"
#  endif /* HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_DEFAULT) & HAL_WATCHDOG_CAPABILITY_NOTIFY */
#  if HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_DEFAULT) & HAL_WATCHDOG_CAPABILITY_STOPPABLE
#   pragma message "HAL_WATCHDOG_CAPABILITY_STOPPABLE"
#  endif /* HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_DEFAULT) & HAL_WATCHDOG_CAPABILITY_STOPPABLE */
# endif /* PLATFORM_WATCHDOG_DEFAULT */

# ifdef PLATFORM_WATCHDOG_INDEPENDENT
#  pragma message "Independent watchdog: enabled"
#  if HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_CPU_RESET
#   pragma message "HAL_WATCHDOG_CAPABILITY_CPU_RESET"
#  endif /* HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_CPU_RESET */
#  if HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_INDEPENDENT
#   pragma message "HAL_WATCHDOG_CAPABILITY_INDEPENDENT"
#  endif /* HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_INDEPENDENT */
#  if HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_WINDOWED
#   pragma message "HAL_WATCHDOG_CAPABILITY_WINDOWED"
#  endif /* HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_WINDOWED */
#  if HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_RECONFIGURABLE
#   pragma message "HAL_WATCHDOG_CAPABILITY_RECONFIGURABLE"
#  endif /* HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_RECONFIGURABLE */
#  if HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_NOTIFY
#   pragma message "HAL_WATCHDOG_CAPABILITY_NOTIFY"
#  endif /* HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_NOTIFY */
#  if HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_STOPPABLE
#   pragma message "HAL_WATCHDOG_CAPABILITY_STOPPABLE"
#  endif /* HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_STOPPABLE */
# endif /* PLATFORM_WATCHDOG_INDEPENDENT */

#endif /* DEBUG_BUILD */

namespace particle {

SystemMonitor* SystemMonitor::instance() {
    static SystemMonitor mon;
    return &mon;
}

SystemMonitor::SystemMonitor() {
    system_monitor_callbacks_t cb = {};
    cb.size = sizeof(cb);
    cb.system_monitor_kick_current = &system_monitor_kick_current;
    cb.system_monitor_get_timeout_current = &system_monitor_get_timeout_current;
    cb.system_monitor_set_timeout_current = &system_monitor_set_timeout_current;
#if defined(MODULE_HAS_SYSTEM_PART1) && MODULE_HAS_SYSTEM_PART1 == 1
    system_monitor_set_callbacks(&cb, nullptr);
#endif /* defined(MODULE_HAS_SYSTEM_PART1) && MODULE_HAS_SYSTEM_PART1 == 1 */
#if defined(MODULE_HAS_SYSTEM_PART3) && MODULE_HAS_SYSTEM_PART3 == 1
    system_monitor_set_callbacks_(&cb, nullptr);
#endif /* defined(MODULE_HAS_SYSTEM_PART3) && MODULE_HAS_SYSTEM_PART3 == 1 */
}

int SystemMonitor::init() {
    os_queue_create(&queue_, sizeof(uintptr_t), 1, nullptr);
    SPARK_ASSERT(queue_ != nullptr);


    os_thread_create(&thread_, "monitor", OS_THREAD_PRIORITY_CRITICAL, [](void* arg) {
        SystemMonitor* self = (SystemMonitor*)arg;
        uintptr_t tmp;

        SPARK_ASSERT(self->watchdogInit());

        while(true) {
            os_queue_take(self->queue_, &tmp, self->tickRate_, nullptr);
            bool kick = true;
            for(const auto& entry: self->entries_) {
                if (entry.taken.load() == true && entry.thread.load() != OS_THREAD_INVALID_HANDLE) {
                    system_tick_t timeout = entry.timeout.load();
                    system_tick_t last = entry.last.load();
                    system_tick_t ms = HAL_Timer_Get_Milli_Seconds();
                    if (ms - last > timeout) {
                        kick = false;
                        break;
                    }
                }
            }

            if (kick) {
                self->watchdogKick();
            } else {
                bool reset = false;
#if (defined(PLATFORM_WATCHDOG_DEFAULT) && HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_DEFAULT) & HAL_WATCHDOG_CAPABILITY_CPU_RESET)
                reset = false;
#elif (defined(PLATFORM_WATCHDOG_INDEPENDENT) && HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_CPU_RESET)
                if (independentEnabled_ == true) {
                    reset = false;
                } else {
                    reset = true;
                }
#else
                reset = true;
#endif
                if (reset) {
                    HAL_Core_System_Reset();
                }
            }
        }

    },
    this, 512);

    SPARK_ASSERT(thread_ != nullptr);

    return 0;
}

void SystemMonitor::notifyCallback(void* arg) {
    SystemMonitor* self = (SystemMonitor*)arg;
    self->notify(PLATFORM_WATCHDOG_DEFAULT);
}

void SystemMonitor::notify(int idx) {
    // TODO: perform a stacktrace dump here
}

void SystemMonitor::sleep(bool state) {
#if defined(PLATFORM_WATCHDOG_INDEPENDENT) && (HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_RECONFIGURABLE)
    hal_watchdog_config_t conf = {};
    conf.size = sizeof(conf);
    if (state) {
        // Going into sleep
        // Reconfigure independent watchdog with the maximum period
        watchdogKick();
        conf.capabilities = HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT);
        conf.period_us = HAL_WATCHDOG_GET_MAX_PERIOD(PLATFORM_WATCHDOG_INDEPENDENT);
        conf.window_us = 0;
        hal_watchdog_configure(PLATFORM_WATCHDOG_INDEPENDENT, &conf, nullptr);
        watchdogKick();
    } else {
        // Waking up
        // Reconfigure independent watchdog with the normal period
        watchdogKick();
        conf.capabilities = HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT);
        conf.period_us = MAX(MIN(WATCHDOG_INDEPENDENT_TIMEOUT_MS * 1000, HAL_WATCHDOG_GET_MAX_PERIOD(PLATFORM_WATCHDOG_INDEPENDENT)), HAL_WATCHDOG_GET_MIN_PERIOD(PLATFORM_WATCHDOG_INDEPENDENT));
        conf.window_us = conf.period_us / 2;
#if HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_NOTIFY
        conf.notify_arg = this;
        conf.notify = SystemMonitor::notifyCallback;
#endif /* HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_NOTIFY */
        hal_watchdog_configure(PLATFORM_WATCHDOG_INDEPENDENT, &conf, nullptr);
        watchdogKick();
    }
#else
    (void)state;
#endif
}

bool SystemMonitor::watchdogInit() {
    system_tick_t tick_rate = SYSTEM_MONITOR_DEFAULT_TICK_RATE_MS;

#ifdef PLATFORM_WATCHDOG_DEFAULT
    {
        hal_watchdog_config_t conf = {};
        conf.size = sizeof(conf);

        conf.capabilities = HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_DEFAULT);
        static constexpr uint32_t period_us = MAX(MIN(WATCHDOG_DEFAULT_TIMEOUT_MS * 1000, HAL_WATCHDOG_GET_MAX_PERIOD(PLATFORM_WATCHDOG_DEFAULT)), HAL_WATCHDOG_GET_MIN_PERIOD(PLATFORM_WATCHDOG_DEFAULT));
        static_assert(period_us >= 10000, "Default watchdog period should be greater than 10ms");
        conf.period_us = period_us;


#if HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_DEFAULT) & HAL_WATCHDOG_CAPABILITY_WINDOWED
        conf.window_us = conf.period_us / 2;
#endif /* HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_DEFAULT) & HAL_WATCHDOG_CAPABILITY_WINDOWED */

#if HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_DEFAULT) & HAL_WATCHDOG_CAPABILITY_NOTIFY
        conf.notify_arg = this;
        conf.notify = SystemMonitor::notifyCallback;
#endif /* HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_DEFAULT) & HAL_WATCHDOG_CAPABILITY_NOTIFY */

        hal_watchdog_configure(PLATFORM_WATCHDOG_DEFAULT, &conf, nullptr);

        hal_watchdog_status_t status = {};
        status.size = sizeof(status);
        hal_watchdog_get_status(PLATFORM_WATCHDOG_DEFAULT, &status, nullptr);

        SPARK_ASSERT(status.period_us != 0);

        if (tick_rate > (status.period_us / 1000)) {
            tick_rate = status.period_us / 1000;
        }
    }
#endif /* PLATFORM_WATCHDOG_DEFAULT */

#ifdef PLATFORM_WATCHDOG_INDEPENDENT
    if (independentEnabled_) {
        hal_watchdog_config_t conf = {};
        conf.size = sizeof(conf);

        conf.capabilities = HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT);
#if HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_RECONFIGURABLE
        // If independent watchdog is reconfigurable, select default timeout
        static constexpr uint32_t period_us = MAX(MIN(WATCHDOG_INDEPENDENT_TIMEOUT_MS * 1000, HAL_WATCHDOG_GET_MAX_PERIOD(PLATFORM_WATCHDOG_INDEPENDENT)), HAL_WATCHDOG_GET_MIN_PERIOD(PLATFORM_WATCHDOG_INDEPENDENT));
#else
        // Otherwise configure maximum timeout
        static constexpr uint32_t period_us = HAL_WATCHDOG_GET_MAX_PERIOD(PLATFORM_WATCHDOG_INDEPENDENT);
#endif /* HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_RECONFIGURABLE */
        static_assert(period_us >= 10000, "Independent watchdog period should be greater than 10ms");
        conf.period_us = period_us;

#if HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_WINDOWED
        conf.window_us = conf.period_us / 2;
#endif /* HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_WINDOWED */

#if HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_NOTIFY
        conf.notify_arg = this;
        conf.notify = SystemMonitor::notifyCallback;
#endif /* HAL_WATCHDOG_GET_CAPABILITIES(PLATFORM_WATCHDOG_INDEPENDENT) & HAL_WATCHDOG_CAPABILITY_NOTIFY */

        hal_watchdog_configure(PLATFORM_WATCHDOG_INDEPENDENT, &conf, nullptr);

        hal_watchdog_status_t status = {};
        status.size = sizeof(status);
        hal_watchdog_get_status(PLATFORM_WATCHDOG_INDEPENDENT, &status, nullptr);

        SPARK_ASSERT(status.period_us != 0);

        if (tick_rate > (status.period_us / 1000)) {
            tick_rate = status.period_us / 1000;
        }
    }
#endif
    initialized_ = true;
    tickRate_ = 3 * tick_rate / 4;

#ifdef PLATFORM_WATCHDOG_DEFAULT
    hal_watchdog_start(PLATFORM_WATCHDOG_DEFAULT, nullptr, nullptr);
#endif
#ifdef PLATFORM_WATCHDOG_INDEPENDENT
    if (independentEnabled_) {
        hal_watchdog_start(PLATFORM_WATCHDOG_INDEPENDENT, nullptr, nullptr);
    }
#endif

    return true;
}

void SystemMonitor::watchdogKick() {
#ifdef PLATFORM_WATCHDOG_DEFAULT
    hal_watchdog_kick(PLATFORM_WATCHDOG_DEFAULT, nullptr);
#endif
#ifdef PLATFORM_WATCHDOG_INDEPENDENT
    if (independentEnabled_) {
        hal_watchdog_kick(PLATFORM_WATCHDOG_INDEPENDENT, nullptr);
    }
#endif
}

int SystemMonitor::enable(os_thread_t thread, system_tick_t timeout) {
    bool set = false;

    if (timeout == 0) {
        timeout = SYSTEM_MONITOR_DEFAULT_THREAD_TIMEOUT_MS;
    }

    for(auto& entry: entries_) {
        bool taken = false;
        if (entry.taken.compare_exchange_strong(taken, true)) {
            set = true;

            entry.timeout.store(timeout);
            entry.last.store(HAL_Timer_Get_Milli_Seconds());

            entry.thread.store(thread);
            break;
        }
    }
    return set ? SYSTEM_ERROR_NONE : SYSTEM_ERROR_LIMIT_EXCEEDED;
}

int SystemMonitor::disable(os_thread_t thread) {
    if (thread == OS_THREAD_INVALID_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    bool set = false;
    for(auto& entry: entries_) {
        os_thread_t t = thread;
        if (entry.taken.load() == true) {
            if (entry.thread.compare_exchange_strong(t, OS_THREAD_INVALID_HANDLE)) {
                set = true;

                entry.last.store(0);
                entry.timeout.store(0);

                entry.taken.store(false);
                break;
            }
        }
    }

    return set ? SYSTEM_ERROR_NONE : SYSTEM_ERROR_NOT_FOUND;
}

int SystemMonitor::kick(os_thread_t thread) {
    if (thread == OS_THREAD_INVALID_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    bool set = false;

    for(auto& entry : entries_) {
        if (entry.thread.load() == thread) {
            set = true;
            entry.last.store(HAL_Timer_Get_Milli_Seconds());
            break;
        }
    }

    return set ? SYSTEM_ERROR_NONE : SYSTEM_ERROR_NOT_FOUND;
}

system_tick_t SystemMonitor::getThreadTimeout(os_thread_t thread) const {
    system_tick_t timeout = 0;

    if (thread == OS_THREAD_INVALID_HANDLE) {
        return timeout;
    }

    for (const auto& entry: entries_) {
        if (entry.thread.load() == thread) {
            timeout = entry.timeout.load();
            break;
        }
    }

    return timeout;
}

int SystemMonitor::setThreadTimeout(os_thread_t thread, system_tick_t timeout) {
    bool set = false;

    if (thread == OS_THREAD_INVALID_HANDLE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    for(auto& entry: entries_) {
        if (entry.thread.load() == thread) {
            entry.timeout.store(timeout);
            set = true;
            break;
        }
    }
    return set ? SYSTEM_ERROR_NONE : SYSTEM_ERROR_NOT_FOUND;
}

system_tick_t SystemMonitor::getMaximumSleepTime() const {
#ifdef PLATFORM_WATCHDOG_INDEPENDENT
    if (independentEnabled_ == true) {
        return HAL_WATCHDOG_GET_MAX_PERIOD(PLATFORM_WATCHDOG_INDEPENDENT) / 1000;
    }
#endif /* PLATFORM_WATCHDOG_INDEPENDENT */

    return 0;
}

int SystemMonitor::configure(system_monitor_configuration_t* conf) {
#ifdef PLATFORM_WATCHDOG_INDEPENDENT
    if (conf && conf->iwdg == 1 && !initialized_) {
        independentEnabled_ = true;
    }
#endif
    return SYSTEM_ERROR_NONE;
}

} /* namespace particle */

#endif /* PLATFORM_THREADING */
