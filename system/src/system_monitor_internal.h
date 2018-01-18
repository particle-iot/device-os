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

#ifndef SYSTEM_MONITOR_INTERNAL_H
#define SYSTEM_MONITOR_INTERNAL_H

#include "system_monitor.h"
#include "watchdog_hal.h"
#include <atomic>

namespace particle {

const size_t SYSTEM_MONITOR_MAX_THREADS = 5;

class SystemMonitor {
public:
    static SystemMonitor* instance();

    int init();

    int enable(os_thread_t thread, system_tick_t timeout);
    int disable(os_thread_t thread);
    int kick(os_thread_t thread);
    system_tick_t getThreadTimeout(os_thread_t thread) const;
    int setThreadTimeout(os_thread_t thread, system_tick_t timeout);
    system_tick_t getMaximumSleepTime() const;
    int configure(system_monitor_configuration_t* conf);

    int sleep(bool state = true);
    int wakeup() {
        return sleep(false);
    }

    int suspend(system_tick_t timeout = 0);
    int resume();

protected:
    SystemMonitor();

    void notify(int idx);
    static void notifyCallback(void* arg);

    struct ThreadEntry {
        std::atomic_bool taken;
        std::atomic<os_thread_t> thread;
        std::atomic<system_tick_t> timeout;
        std::atomic<system_tick_t> last;
    };

    enum EventType {
        MONITOR_EVENT_TYPE_NONE = 0,
        MONITOR_EVENT_TYPE_SLEEP,
        MONITOR_EVENT_TYPE_SUSPEND,
        MONITOR_EVENT_TYPE_RESUME
    };

    struct Event {
        Event() = default;
        Event(EventType type, void* data);
        ~Event();

        void notify();
        void wait();
        void clear();

        EventType type = MONITOR_EVENT_TYPE_NONE;
        os_semaphore_t sem = nullptr;
        void* data = nullptr;
    };

    using SleepEventData = bool*;
    using SuspendEventData = system_tick_t*;
    using ResumeEventData = void*;

private:
    bool watchdogInit();
    bool watchdogConfigure(int idx, system_tick_t period_us, bool ifGreater = false, bool closest = true);
    bool watchdogStart(int idx);
    bool watchdogStop(int idx);
    bool watchdogSuspend(int idx, system_tick_t timeout);
    bool watchdogResume(int idx);
    void watchdogKick();
    hal_watchdog_info_t watchdogInfo(int idx, bool& ok) const;
    hal_watchdog_status_t watchdogStatus(int idx, bool& ok) const;

    void postEvent(EventType type, void* data);
    void handleEvent(Event& event);

    void sleepImpl(bool state, system_tick_t timeout = 0);
    void suspendImpl(bool state, system_tick_t timeout = 0);

private:
    os_thread_t thread_ = {};
    os_queue_t queue_ = {};
    ThreadEntry entries_[SYSTEM_MONITOR_MAX_THREADS] = {};
    system_tick_t tickRate_ = 0;
    bool enabled_ = false;
    bool initialized_ = false;
    bool independentEnabled_ = false;
    int suspendRefCount_ = 0;
    bool manualReset_ = true;
};

} /* namespace particle */

#endif /* SYSTEM_MONITOR_INTERNAL_H */
