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

#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <stdint.h>
#include "concurrent_hal.h"
#include "system_tick_hal.h"
#include "monitor_service.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    uint16_t size;
    uint16_t version;

    union {
        uint32_t flags;
        uint32_t iwdg : 1;
    };
} system_monitor_configuration_t;

// Enables monitoring for the specified thread with a specified period
int system_monitor_enable(os_thread_t thread, system_tick_t timeout, void* reserved);

// Disables monitoring for the specified thread
int system_monitor_disable(os_thread_t thread, void* reserved);

// Reports that the specified thread is alive
int system_monitor_kick(os_thread_t thread, void* reserved);

// Reports that the current thread is alive
int system_monitor_kick_current(void* reserved);

// Gets the timeout for the specified thread. Returns 0 if monitoring is not
// enabled for the specified thread.
system_tick_t system_monitor_get_thread_timeout(os_thread_t thread, void* reserved);

// Changes the timeout for the specified thread. Returns an error if monitoring is not
// enabled for the specified thread.
int system_monitor_set_thread_timeout(os_thread_t thread, system_tick_t timeout, void* reserved);

// Gets the timeout for the current thread. Returns 0 if monitoring is not
// enabled for the current thread.
system_tick_t system_monitor_get_timeout_current(void* reserved);

// Changes the timeout for the current thread. Returns an error if monitoring is not
// enabled for the current thread.
int system_monitor_set_timeout_current(system_tick_t timeout, void* reserved);

// Returns maximum amount of time the device may sleep, or 0 if indefinitely
system_tick_t system_monitor_get_max_sleep_time(void* reserved, void* reserved1);

// Updates monitoring settings
int system_monitor_configure(system_monitor_configuration_t* conf, void* reserved);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SYSTEM_MONITOR_H */
