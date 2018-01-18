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

#include "module_info.h"
#include "watchdog_hal.h"

#if defined(HAL_WATCHDOG_COUNT) && PLATFORM_THREADING && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
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
typedef int (*system_monitor_suspend_callback_t)(system_tick_t, void*);
typedef int (*system_monitor_resume_callback_t)(void*);

typedef struct {
    uint16_t size;
    uint16_t version;

    system_monitor_kick_current_callback_t system_monitor_kick_current;
    system_monitor_get_timeout_current_callback_t system_monitor_get_timeout_current;
    system_monitor_set_timeout_current_callback_t system_monitor_set_timeout_current;
    system_monitor_suspend_callback_t system_monitor_suspend;
    system_monitor_resume_callback_t system_monitor_resume;
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

int system_monitor_suspend(system_tick_t timeout, void* reserved);
int system_monitor_suspend_(system_tick_t timeout, void* reserved);
int system_monitor_suspend__(system_tick_t timeout, void* reserved);

int system_monitor_resume(void* reserved);
int system_monitor_resume_(void* reserved);
int system_monitor_resume__(void* reserved);

#if SYSTEM_MONITOR_ENABLED == 1
# if defined(SYSTEM_MONITOR_USE_CALLBACKS)
#  define SYSTEM_MONITOR_KICK_CURRENT() system_monitor_kick_current_(NULL)
#  define SYSTEM_MONITOR_GET_TIMEOUT_CURRENT() system_monitor_get_timeout_current_(NULL)
#  define SYSTEM_MONITOR_SET_TIMEOUT_CURRENT(timeout) system_monitor_set_timeout_current_(timeout, NULL)
#  define SYSTEM_MONITOR_SUSPEND(timeout) system_monitor_suspend_(timeout, NULL)
#  define SYSTEM_MONITOR_RESUME() system_monitor_resume_(NULL)
# elif defined(SYSTEM_MONITOR_USE_CALLBACKS2)
#  define SYSTEM_MONITOR_KICK_CURRENT() system_monitor_kick_current__(NULL)
#  define SYSTEM_MONITOR_GET_TIMEOUT_CURRENT() system_monitor_get_timeout_current__(NULL)
#  define SYSTEM_MONITOR_SET_TIMEOUT_CURRENT(timeout) system_monitor_set_timeout_current__(timeout, NULL)
#  define SYSTEM_MONITOR_SUSPEND(timeout) system_monitor_suspend__(timeout, NULL)
#  define SYSTEM_MONITOR_RESUME() system_monitor_resume__(NULL)
# else
#  define SYSTEM_MONITOR_KICK_CURRENT() system_monitor_kick_current(NULL)
#  define SYSTEM_MONITOR_GET_TIMEOUT_CURRENT() system_monitor_get_timeout_current(NULL)
#  define SYSTEM_MONITOR_SET_TIMEOUT_CURRENT(timeout) system_monitor_set_timeout_current(timeout, NULL)
#  define SYSTEM_MONITOR_SUSPEND(timeout) system_monitor_suspend(timeout, NULL)
#  define SYSTEM_MONITOR_RESUME() system_monitor_resume(NULL)
# endif
#else /* SYSTEM_MONITOR_ENABLED == 1 */
# define SYSTEM_MONITOR_KICK_CURRENT()
# define SYSTEM_MONITOR_GET_TIMEOUT_CURRENT() (0)
# define SYSTEM_MONITOR_SET_TIMEOUT_CURRENT(timeout)
# define SYSTEM_MONITOR_SUSPEND(timeout)
# define SYSTEM_MONITOR_RESUME()
#endif /* SYSTEM_MONITOR_ENABLED == 1 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __cplusplus

#if SYSTEM_MONITOR_ENABLED == 1
struct SystemMonitorTimeoutHelper {
    SystemMonitorTimeoutHelper(system_tick_t timeout, bool stall = false) {
        saved_ = SYSTEM_MONITOR_GET_TIMEOUT_CURRENT();
        stall_ = stall;
        if (timeout > 0) {
            SYSTEM_MONITOR_SET_TIMEOUT_CURRENT(timeout);
        }
        SYSTEM_MONITOR_KICK_CURRENT();
        if (stall_) {
            SYSTEM_MONITOR_SUSPEND(timeout);
        }
    }

    ~SystemMonitorTimeoutHelper() {
        SYSTEM_MONITOR_KICK_CURRENT();
        if (saved_ != 0) {
            SYSTEM_MONITOR_SET_TIMEOUT_CURRENT(saved_);
        }
        if (stall_) {
            SYSTEM_MONITOR_RESUME();
        }
    }

private:
    system_tick_t saved_ = 0;
    bool stall_ = false;
};

# define SYSTEM_MONITOR_MODIFY_TIMEOUT(timeout) SystemMonitorTimeoutHelper helper ## __COUNTER__ (timeout)
# define SYSTEM_MONITOR_EXPECT_STALL(timeout) SystemMonitorTimeoutHelper helper ## __COUNTER__ (timeout, true)

#else

# define SYSTEM_MONITOR_MODIFY_TIMEOUT(timeout)
# define SYSTEM_MONITOR_EXPECT_STALL(timeout)

#endif /* SYSTEM_MONITOR_ENABLED == 1 */

#endif /* __cplusplus */

#endif /* SERVICES_MONITOR_SERVICE_H */
