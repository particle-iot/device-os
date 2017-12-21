/**
 ******************************************************************************
 * @file    watchdog_hal.h
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    25-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

#ifndef WATCHDOG_HAL_H
#define	WATCHDOG_HAL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

bool HAL_watchdog_reset_flagged();

void HAL_Notify_WDT();

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

#define HAL_WATCHDOG_GET_PARAM_IMPL1(wd, param)     PLATFORM_WATCHDOG_ ## wd ## _ ## param
#define HAL_WATCHDOG_GET_PARAM_IMPL2(wd, param)     HAL_WATCHDOG_GET_PARAM_IMPL1(wd, param)
#define HAL_WATCHDOG_GET_PARAM(wd, param)           HAL_WATCHDOG_GET_PARAM_IMPL2(wd, param)

#define HAL_WATCHDOG_GET_CAPABILITIES(wd)           (HAL_WATCHDOG_GET_PARAM(wd, CAPABILITIES))
#define HAL_WATCHDOG_GET_MIN_PERIOD(wd)             (HAL_WATCHDOG_GET_PARAM(wd, MIN_PERIOD_US))
#define HAL_WATCHDOG_GET_MAX_PERIOD(wd)             (HAL_WATCHDOG_GET_PARAM(wd, MAX_PERIOD_US))

typedef struct {
    uint16_t size;
    uint16_t version;

    uint8_t running;
    uint32_t capabilities;
    uint32_t period_us;
    uint32_t window_us;
} hal_watchdog_status_t;

typedef struct {
    uint16_t size;
    uint16_t version;

    uint32_t capabilities;
    uint32_t period_us;
    uint32_t window_us;

    void (*notify)(void*);
    void* notify_arg;
} hal_watchdog_config_t;

int hal_watchdog_start(int idx, hal_watchdog_config_t* conf, void* reserved);

int hal_watchdog_configure(int idx, hal_watchdog_config_t* conf, void* reserved);

int hal_watchdog_stop(int idx, void* reserved);

int hal_watchdog_get_status(int idx, hal_watchdog_status_t* status, void* reserved);

int hal_watchdog_kick(int idx, void* reserved);

#ifdef	__cplusplus
}
#endif

#endif	/* WATCHDOG_HAL_H */

