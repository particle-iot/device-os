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

#ifndef SERVICES_MONITOR_SERVICE_H
#define SERVICES_MONITOR_SERVICE_H

#include <stddef.h>
#include "system_tick_hal.h"

#ifndef SPARK_PLATFORM
#define SYSTEM_MONITOR_SKIP_PLATFORM
#endif /* SPARK_PLATFORM */

#ifndef SYSTEM_MONITOR_SKIP_PLATFORM
#include "platform_config.h"
#endif

#if defined(PLATFORM_WATCHDOG_COUNT) && PLATFORM_THREADING
# define SYSTEM_MONITOR_ENABLED (1)
#else
# define SYSTEM_MONITOR_ENABLED (0)
#endif /* defined(PLATFORM_WATCHDOG_COUNT) && PLATFORM_THREADING */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef int (*system_monitor_kick_current_callback_t)(void*);
typedef system_tick_t (*system_monitor_get_timeout_current_callback_t)(void*);
typedef int (*system_monitor_set_timeout_current_callback_t)(system_tick_t, void*);

typedef struct {
    uint16_t size;
    uint16_t version;

    system_monitor_kick_current_callback_t system_monitor_kick_current;
    system_monitor_get_timeout_current_callback_t system_monitor_get_timeout_current;
    system_monitor_set_timeout_current_callback_t system_monitor_set_timeout_current;
} system_monitor_callbacks_t;

int system_monitor_set_callbacks(system_monitor_callbacks_t* callbacks, void* reserved);
int system_monitor_set_callbacks_(system_monitor_callbacks_t* callbacks, void* reserved);

int system_monitor_kick_current(void* reserved);
int system_monitor_kick_current_(void* reserved);
int system_monitor_kick_current__(void* reserved);

system_tick_t system_monitor_get_timeout_current(void* reserved);
int system_monitor_set_timeout_current(system_tick_t timeout, void* reserved);

system_tick_t system_monitor_get_timeout_current_(void* reserved);
int system_monitor_set_timeout_current_(system_tick_t timeout, void* reserved);

system_tick_t system_monitor_get_timeout_current__(void* reserved);
int system_monitor_set_timeout_current__(system_tick_t timeout, void* reserved);

#if SYSTEM_MONITOR_ENABLED == 1
# if defined(SYSTEM_MONITOR_USE_CALLBACKS)
#  define SYSTEM_MONITOR_KICK_CURRENT() system_monitor_kick_current_(NULL)
#  define SYSTEM_MONITOR_GET_TIMEOUT_CURRENT() system_monitor_get_timeout_current_(NULL)
#  define SYSTEM_MONITOR_SET_TIMEOUT_CURRENT(timeout) system_monitor_set_timeout_current_(timeout, NULL)
# elif defined(SYSTEM_MONITOR_USE_CALLBACKS2)
#  define SYSTEM_MONITOR_KICK_CURRENT() system_monitor_kick_current__(NULL)
#  define SYSTEM_MONITOR_GET_TIMEOUT_CURRENT() system_monitor_get_timeout_current__(NULL)
#  define SYSTEM_MONITOR_SET_TIMEOUT_CURRENT(timeout) system_monitor_set_timeout_current__(timeout, NULL)
# else
#  define SYSTEM_MONITOR_KICK_CURRENT() system_monitor_kick_current(NULL)
#  define SYSTEM_MONITOR_GET_TIMEOUT_CURRENT() system_monitor_get_timeout_current(NULL)
#  define SYSTEM_MONITOR_SET_TIMEOUT_CURRENT(timeout) system_monitor_set_timeout_current(timeout, NULL)
# endif
#else /* SYSTEM_MONITOR_ENABLED == 1 */
# define SYSTEM_MONITOR_KICK_CURRENT()
# define SYSTEM_MONITOR_GET_TIMEOUT_CURRENT() (0)
# define SYSTEM_MONITOR_SET_TIMEOUT_CURRENT(timeout)
#endif /* SYSTEM_MONITOR_ENABLED == 1 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __cplusplus

struct SystemMonitorTimeoutHelper {
    SystemMonitorTimeoutHelper(system_tick_t timeout) {
        saved_ = SYSTEM_MONITOR_GET_TIMEOUT_CURRENT();
        if (timeout > 0) {
            SYSTEM_MONITOR_SET_TIMEOUT_CURRENT(timeout);
        }
        SYSTEM_MONITOR_KICK_CURRENT();
    }

    ~SystemMonitorTimeoutHelper() {
        SYSTEM_MONITOR_KICK_CURRENT();
        if (saved_ != 0) {
            SYSTEM_MONITOR_SET_TIMEOUT_CURRENT(saved_);
        }
    }

private:
    system_tick_t saved_ = 0;
};

#define SYSTEM_MONITOR_MODIFY_TIMEOUT(timeout) SystemMonitorTimeoutHelper helper ## __COUNTER__ (timeout)

#endif /* __cplusplus */

#endif /* SERVICES_MONITOR_SERVICE_H */
