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

#include "system_monitor.h"
#include "system_monitor_internal.h"

using namespace particle;

#if PLATFORM_THREADING

int system_monitor_enable(os_thread_t thread, system_tick_t timeout, void* reserved) {
    return SystemMonitor::instance()->enable(thread, timeout);
}

int system_monitor_disable(os_thread_t thread, void* reserved) {
    return SystemMonitor::instance()->disable(thread);
}

int system_monitor_kick(os_thread_t thread, void* reserved) {
    return SystemMonitor::instance()->kick(thread);
}

int system_monitor_kick_current(void* reserved) {
    return system_monitor_kick(os_thread_current(), reserved);
}

system_tick_t system_monitor_get_thread_timeout(os_thread_t thread, void* reserved) {
    return SystemMonitor::instance()->getThreadTimeout(thread);
}

int system_monitor_set_thread_timeout(os_thread_t thread, system_tick_t timeout, void* reserved) {
    return SystemMonitor::instance()->setThreadTimeout(thread, timeout);
}

system_tick_t system_monitor_get_max_sleep_time(void* reserved, void* reserved1) {
    return SystemMonitor::instance()->getMaximumSleepTime();
}

int system_monitor_configure(system_monitor_configuration_t* conf, void* reserved) {
    return SystemMonitor::instance()->configure(conf);
}

system_tick_t system_monitor_get_timeout_current(void* reserved) {
    return system_monitor_get_thread_timeout(os_thread_current(), reserved);
}

int system_monitor_set_timeout_current(system_tick_t timeout, void* reserved) {
    return system_monitor_set_thread_timeout(os_thread_current(), timeout, reserved);
}

#endif /* PLATFORM_THREADING */
