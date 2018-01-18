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
#include "spark_wiring_interrupts.h"
#include "system_error.h"
#include "timer_hal.h"
#include "core_hal.h"
#include "debug.h"
#include <limits>

#if PLATFORM_THREADING

namespace {
// 100ms
const system_tick_t WATCHDOG_DEFAULT_TIMEOUT_MS = 100;
// 60ms
const system_tick_t SYSTEM_MONITOR_DEFAULT_TICK_RATE_MS = 60;
// 1 second
const system_tick_t WATCHDOG_INDEPENDENT_TIMEOUT_MS = 1000;

const system_tick_t SYSTEM_MONITOR_DEFAULT_THREAD_TIMEOUT_MS = 1000;

} /* anonymous namespace */

namespace particle {

SystemMonitor::Event::Event(SystemMonitor::EventType t, void* d)
        : type(t),
          data(d) {
    if (!HAL_IsISR()) {
        os_semaphore_create(&sem, 1, 0);
    }
}

SystemMonitor::Event::~Event() {
    if (sem != nullptr) {
        os_semaphore_destroy(sem);
    }
}

void SystemMonitor::Event::wait() {
    os_semaphore_take(sem, CONCURRENT_WAIT_FOREVER, false);
}

void SystemMonitor::Event::notify() {
    if (sem != nullptr) {
        os_semaphore_give(sem, true);
    }
}

void SystemMonitor::Event::clear() {
    sem = nullptr;
}

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
    cb.system_monitor_suspend = &system_monitor_suspend;
    cb.system_monitor_resume = &system_monitor_resume;
// #if defined(MODULE_HAS_SYSTEM_PART1) && MODULE_HAS_SYSTEM_PART1 == 1
    system_monitor_set_callbacks(&cb, nullptr);
// #endif /* defined(MODULE_HAS_SYSTEM_PART1) && MODULE_HAS_SYSTEM_PART1 == 1 */
#if defined(MODULE_HAS_SYSTEM_PART3) && MODULE_HAS_SYSTEM_PART3 == 1
    system_monitor_set_callbacks_(&cb, nullptr);
#endif /* defined(MODULE_HAS_SYSTEM_PART3) && MODULE_HAS_SYSTEM_PART3 == 1 */
    tickRate_ = SYSTEM_MONITOR_DEFAULT_TICK_RATE_MS;
}

int SystemMonitor::init() {
    if (!enabled_) {
        return -1;
    }

    os_queue_create(&queue_, sizeof(Event), 1, nullptr);
    SPARK_ASSERT(queue_ != nullptr);


    os_thread_create(&thread_, "monitor", OS_THREAD_PRIORITY_CRITICAL, [](void* arg) {
        SystemMonitor* self = (SystemMonitor*)arg;

        SPARK_ASSERT(self->watchdogInit());

        while(true) {
            Event event;
            os_queue_take(self->queue_, &event, self->tickRate_, nullptr);

            if (event.type != MONITOR_EVENT_TYPE_NONE) {
                self->handleEvent(event);
            }

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
                if (self->manualReset_) {
                    HAL_Core_System_Reset();
                }
            }
        }

    },
    this, 512);

    SPARK_ASSERT(thread_ != nullptr);

    return 0;
}

void SystemMonitor::handleEvent(Event& event) {
    int32_t state = HAL_disable_irq();
    switch (event.type) {
        case MONITOR_EVENT_TYPE_SLEEP: {
            SleepEventData data = static_cast<SleepEventData>(event.data);
            sleepImpl(*data);
            break;
        }

        case MONITOR_EVENT_TYPE_SUSPEND: {
            SuspendEventData data = static_cast<SuspendEventData>(event.data);
            suspendImpl(true, *data);
            break;
        }

        case MONITOR_EVENT_TYPE_RESUME: {
            suspendImpl(false);
        }
    }
    HAL_enable_irq(state);

    event.notify();
    event.clear();
}

void SystemMonitor::notifyCallback(void* arg) {
    SystemMonitor* self = (SystemMonitor*)arg;
    // FIXME: correct id
    self->notify(0);
}

void SystemMonitor::notify(int idx) {
    // TODO: perform a stacktrace dump here
}

void SystemMonitor::postEvent(EventType type, void* data) {
    if (!enabled_) {
        return;
    }
    Event ev(type, data);
    if (!HAL_IsISR()) {
        os_queue_put(queue_, &ev, CONCURRENT_WAIT_FOREVER, nullptr);
        ev.wait();
    } else {
        handleEvent(ev);
    }
}

int SystemMonitor::sleep(bool state) {
    postEvent(MONITOR_EVENT_TYPE_SLEEP, &state);
    return 0;
}

int SystemMonitor::suspend(system_tick_t timeout) {
    postEvent(MONITOR_EVENT_TYPE_SUSPEND, &timeout);
    return 0;
}

int SystemMonitor::resume() {
    postEvent(MONITOR_EVENT_TYPE_RESUME, nullptr);
    return 0;
}

void SystemMonitor::suspendImpl(bool state, system_tick_t timeout) {
    const system_tick_t timeout_us = timeout * 1000;

    if (state) {
        // Suspend
        ++suspendRefCount_;

        watchdogKick();

#ifdef HAL_WATCHDOG_DEFAULT
        SPARK_ASSERT(watchdogSuspend(HAL_WATCHDOG_DEFAULT, timeout_us));
#endif /* HAL_WATCHDOG_DEFAULT */

#ifdef HAL_WATCHDOG_INDEPENDENT
        SPARK_ASSERT(watchdogSuspend(HAL_WATCHDOG_INDEPENDENT, timeout_us));
#endif /* HAL_WATCHDOG_INDEPENDENT */

        watchdogKick();
    } else {
        // Resume
        if (--suspendRefCount_ == 0) {
            // Restore default settings
            watchdogKick();
            watchdogInit();
            watchdogKick();
        }
    }
}

void SystemMonitor::sleepImpl(bool state, system_tick_t timeout) {
#ifdef HAL_WATCHDOG_INDEPENDENT
    hal_watchdog_config_t conf = {};
    conf.size = sizeof(conf);
    if (state) {
        // Going into sleep
        // Reconfigure independent watchdog with the maximum period
        watchdogKick();
        watchdogConfigure(HAL_WATCHDOG_INDEPENDENT, std::numeric_limits<system_tick_t>::max(), false, true);
        watchdogKick();
    } else {
        // Waking up
        // Reconfigure watchdogs with the normal period
        watchdogKick();
        watchdogInit();
        watchdogKick();
    }
#else
    (void)state;
#endif /* HAL_WATCHDOG_INDEPENDENT */
}

bool SystemMonitor::watchdogConfigure(int idx, system_tick_t period_us, bool ifGreater, bool closest) {
    {
        bool ok = false;
        auto status = watchdogStatus(idx, ok);

        if (ok && status.running && !(status.capabilities & HAL_WATCHDOG_CAPABILITY_RECONFIGURABLE)) {
            return false;
        }

        if (status.running && ifGreater) {
            if (period_us < status.period_us) {
                return false;
            }
        }
    }

    bool ok = false;

    auto info = watchdogInfo(idx, ok);
    if (!ok) {
        return false;
    }

    hal_watchdog_config_t conf = {};
    conf.size = sizeof(conf);

    conf.capabilities = info.capabilities;
    if (period_us <= info.max_period_us || closest)  {
        conf.period_us = std::max(std::min(period_us, info.max_period_us), info.min_period_us);
    } else {
        return false;
    }

    if (info.capabilities & HAL_WATCHDOG_CAPABILITY_WINDOWED) {
        // Window is disabled
        conf.window_us = 0;
    }

    if (info.capabilities & HAL_WATCHDOG_CAPABILITY_NOTIFY) {
        conf.notify_arg = this;
        conf.notify = SystemMonitor::notifyCallback;
    }

    hal_watchdog_configure(idx, &conf, nullptr);

    ok = false;
    auto status = watchdogStatus(idx, ok);

    SPARK_ASSERT(ok == true && status.period_us != 0);

    if (manualReset_ == true) {
        if (info.capabilities & HAL_WATCHDOG_CAPABILITY_CPU_RESET) {
            manualReset_ = false;
        }
    }

    const system_tick_t tickRate = (status.period_us / 1000) / 2;

    if (tickRate_ > tickRate) {
        tickRate_ = tickRate;
    }

    return true;
}

bool SystemMonitor::watchdogSuspend(int idx, system_tick_t timeout_us) {
    if (timeout_us == 0) {
        // Indefinite
        if (!watchdogStop(idx)) {
            // If watchdog cannot be stopped, reconfigure it with the longest timeout
            return watchdogConfigure(idx, std::numeric_limits<system_tick_t>::max(), true, true);
        }
    } else {
        if (!watchdogConfigure(idx, timeout_us, true, false)) {
            // If watchdog cannot be suspended for such a period of time - attempt to stop it
            return watchdogStop(idx);
        }
    }

    return true;
}

bool SystemMonitor::watchdogInit() {
#ifdef HAL_WATCHDOG_DEFAULT
    watchdogConfigure(HAL_WATCHDOG_DEFAULT, WATCHDOG_DEFAULT_TIMEOUT_MS * 1000);
#endif /* HAL_WATCHDOG_DEFAULT */

#ifdef HAL_WATCHDOG_INDEPENDENT
    if (independentEnabled_) {
        watchdogConfigure(HAL_WATCHDOG_INDEPENDENT, WATCHDOG_INDEPENDENT_TIMEOUT_MS * 1000);
    }
#endif /* HAL_WATCHDOG_DEFAULT */

    initialized_ = true;

#ifdef HAL_WATCHDOG_DEFAULT
    hal_watchdog_start(HAL_WATCHDOG_DEFAULT, nullptr, nullptr);
#endif /* HAL_WATCHDOG_DEFAULT */

#ifdef HAL_WATCHDOG_INDEPENDENT
    if (independentEnabled_) {
        hal_watchdog_start(HAL_WATCHDOG_INDEPENDENT, nullptr, nullptr);
    }
#endif /* HAL_WATCHDOG_INDEPENDENT */

    return true;
}

void SystemMonitor::watchdogKick() {
#ifdef HAL_WATCHDOG_DEFAULT
    hal_watchdog_kick(HAL_WATCHDOG_DEFAULT, nullptr);
#endif /* HAL_WATCHDOG_DEFAULT */
#ifdef HAL_WATCHDOG_INDEPENDENT
    if (independentEnabled_) {
        hal_watchdog_kick(HAL_WATCHDOG_INDEPENDENT, nullptr);
    }
#endif /* HAL_WATCHDOG_INDEPENDENT */
}

bool SystemMonitor::watchdogStart(int idx) {
    return hal_watchdog_start(idx, nullptr, nullptr) == 0;
}

bool SystemMonitor::watchdogStop(int idx) {
    return hal_watchdog_stop(idx, nullptr) == 0;
}

hal_watchdog_info_t SystemMonitor::watchdogInfo(int idx, bool& ok) const {
    hal_watchdog_info_t info = {};
    info.size = sizeof(info);

    ok = hal_watchdog_query(idx, &info, nullptr) == 0;

    return info;
}

hal_watchdog_status_t SystemMonitor::watchdogStatus(int idx, bool& ok) const {
    hal_watchdog_status_t status = {};
    status.size = sizeof(status);

    ok = hal_watchdog_get_status(idx, &status, nullptr) == 0;

    return status;
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
        if (HAL_IsISR()) {
            // Directly kick hardware watchdogs
            watchdogKick();
            return SYSTEM_ERROR_NONE;
        }
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
#ifdef HAL_WATCHDOG_INDEPENDENT
    if (enabled_ && independentEnabled_ == true) {
        bool ok = false;
        auto info = watchdogInfo(HAL_WATCHDOG_INDEPENDENT, ok);
        if (ok) {
            return info.max_period_us / 1000;
        }
    }
#endif /* HAL_WATCHDOG_INDEPENDENT */

    return 0;
}

int SystemMonitor::configure(system_monitor_configuration_t* conf) {
#ifdef HAL_WATCHDOG_INDEPENDENT
    if (conf && conf->iwdg == 1 && !initialized_) {
        independentEnabled_ = true;
    }
    if (conf && conf->enabled == 1 && !initialized_) {
        enabled_ = true;
    }
#endif /* HAL_WATCHDOG_INDEPENDENT */
    return SYSTEM_ERROR_NONE;
}

} /* namespace particle */

#endif /* PLATFORM_THREADING */
