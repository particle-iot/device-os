/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#ifndef WATCHDOG_HAL_H
#define	WATCHDOG_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include "hal_platform.h"
#include "system_tick_hal.h"

#define HAL_WATCHDOG_VERSION    1

typedef void (*hal_watchdog_on_expired_callback_t)(void* context);

typedef enum hal_watchdog_instance_t {
    HAL_WATCHDOG_INSTANCE1 = 0,
    HAL_WATCHDOG_INSTANCE2 = 1,
    HAL_WATCHDOG_INSTANCE_ALL = 0xFF,
} hal_watchdog_instance_t;

typedef enum hal_watchdog_capability_t {
    HAL_WATCHDOG_CAPS_NONE              = 0x00,
    HAL_WATCHDOG_CAPS_RESET             = 0x01,
    HAL_WATCHDOG_CAPS_NOTIFY            = 0x02,
    HAL_WATCHDOG_CAPS_NOTIFY_ONLY       = 0x04,
    HAL_WATCHDOG_CAPS_RECONFIGURABLE    = 0x08,
    HAL_WATCHDOG_CAPS_STOPPABLE         = 0x10,
    HAL_WATCHDOG_CAPS_SLEEP_RUNNING     = 0x20,
    HAL_WATCHDOG_CAPS_DEBUG_RUNNING     = 0x40,
    HAL_WATCHDOG_CAPS_ALL               = 0xFFFFFFFF
} hal_watchdog_capability_t;

typedef enum hal_watchdog_state_t {
    HAL_WATCHDOG_STATE_DISABLED         = 0x00,
    HAL_WATCHDOG_STATE_CONFIGURED       = 0x01,
    HAL_WATCHDOG_STATE_STARTED          = 0x02,
    HAL_WATCHDOG_STATE_SUSPENDED        = 0x03,
    HAL_WATCHDOG_STATE_STOPPED          = 0x04
} hal_watchdog_state_t;

typedef struct hal_watchdog_config_t {
    uint16_t                            size;
    uint16_t                            version;
    system_tick_t                       timeout_ms;
    uint32_t                            enable_caps;
} hal_watchdog_config_t;

typedef struct hal_watchdog_info_t {
    uint16_t                    size;
    uint16_t                    version;
    uint32_t                    mandatory_caps;
    uint32_t                    optional_caps;
    hal_watchdog_config_t       config;
    system_tick_t               min_timeout_ms;
    system_tick_t               max_timeout_ms;
    hal_watchdog_state_t        state;
    uint8_t                     reserved[3];
} hal_watchdog_info_t;

#ifdef	__cplusplus
extern "C" {
#endif

int hal_watchdog_set_config(hal_watchdog_instance_t instance, const hal_watchdog_config_t* config, void* reserved);
int hal_watchdog_on_expired_callback(hal_watchdog_instance_t instance, hal_watchdog_on_expired_callback_t callback, void* context, void* reserved);
int hal_watchdog_start(hal_watchdog_instance_t instance, void* reserved);
int hal_watchdog_stop(hal_watchdog_instance_t instance, void* reserved);
int hal_watchdog_refresh(hal_watchdog_instance_t instance, void* reserved);
int hal_watchdog_get_info(hal_watchdog_instance_t instance, hal_watchdog_info_t* info, void* reserved);

// Dynalib backwards compatible
bool hal_watchdog_reset_flagged_deprecated();
void hal_watchdog_refresh_deprecated(void);

#ifdef	__cplusplus
}
#endif

#endif	/* WATCHDOG_HAL_H */

