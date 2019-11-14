/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
 
 #ifndef __SLEEP_HAL_H
 #define __SLEEP_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "system_tick_hal.h"
#include "interrupts_hal.h"

#define HAL_SLEEP_VERSION 2

/**
 * Stop mode:
 *     Only the wakyup sources specified by user are enabled during in stop mode.
 *     Other peripherals will be temporarily disabled, but their configurations are cached
 *     before entering stop mode.
 *     Flash and RAM memory are always on during this mode.
 *     System clocks that are utilized to drive the specified wakeup sources remain working,
 *     otherwise, they are disabled for saving power.
 *     Once device is wakeup, device restores peripherals' configurations and connitue running as normal.
 * 
 * Hibernate mode:
 *     Only some of the wakeup sources are available to wakeup device.
 *     Otehr peripherals are completely shut off, without caching their configurations.
 *     Flash and RAM memory are disabled, while the retention RAM keeps on to store power-cycle data.
 *     Once device is wakeup, it performs a reset.
 * 
 * Shutdown mode:
 *     Only certain GPIO can be the wakeup source.
 *     CPU, every peripherals, system clock and all flash and RAM memory are completely shut off.
 *     Once device is wakeup, it performs a reset.
 */
typedef enum hal_sleep_mode_t {
    HAL_SLEEP_MODE_NONE = 0,
    HAL_SLEEP_MODE_STOP = 1,
    HAL_SLEEP_MODE_HIBERNATE = 2,
    HAL_SLEEP_MODE_SHUTDOWN = 3,
    HAL_SLEEP_MODE_MAX = 0x7F
} hal_sleep_mode_t;
static_assert(sizeof(hal_sleep_mode_t) == 1, "length of hal_sleep_mode_t should be 1-bytes aligned.");

typedef enum hal_wakeup_source_type_t {
    HAL_WAKEUP_SOURCE_TYPE_UNKNOWN = 0,
    HAL_WAKEUP_SOURCE_TYPE_GPIO = 1,
    HAL_WAKEUP_SOURCE_TYPE_ADC = 2,
    HAL_WAKEUP_SOURCE_TYPE_DAC = 3,
    HAL_WAKEUP_SOURCE_TYPE_RTC = 4,
    HAL_WAKEUP_SOURCE_TYPE_LPCOMP = 5,
    HAL_WAKEUP_SOURCE_TYPE_UART = 6,
    HAL_WAKEUP_SOURCE_TYPE_I2C = 7,
    HAL_WAKEUP_SOURCE_TYPE_SPI = 8,
    HAL_WAKEUP_SOURCE_TYPE_TIMER = 9,
    HAL_WAKEUP_SOURCE_TYPE_CAN = 10,
    HAL_WAKEUP_SOURCE_TYPE_USB = 11,
    HAL_WAKEUP_SOURCE_TYPE_BLE = 12,
    HAL_WAKEUP_SOURCE_TYPE_NFC = 13,
    HAL_WAKEUP_SOURCE_TYPE_NETWORK = 14,
    HAL_WAKEUP_SOURCE_TYPE_MAX = 0x7FFF
} hal_wakeup_source_type_t;
static_assert(sizeof(hal_wakeup_source_type_t) == 2, "length of hal_wakeup_source_type_t should be 2-bytes aligned.");

/**
 * HAL sleep wakeup source base
 */
typedef struct hal_wakeup_source_base_t {
    uint16_t size;
    uint16_t version;
    hal_wakeup_source_type_t type;
    uint16_t reserved;
    hal_wakeup_source_base_t* next;
} hal_wakeup_source_base_t;

/**
 * HAL sleep wakeup source: GPIO
 */
typedef struct hal_wakeup_source_gpio_t {
    hal_wakeup_source_base_t base;
    uint16_t pin;
    InterruptMode mode; // Caution: This might not be 1-byte length, depending on linker options.
    uint8_t reserved;
} hal_wakeup_source_gpio_t;

/**
 * HAL sleep wakeup source: RTC
 */
typedef struct hal_wakeup_source_rtc_t {
    hal_wakeup_source_base_t base;
    system_tick_t ms;
} hal_wakeup_source_rtc_t;

/**
 * HAL sleep configuration: speicify sleep mode and wakeup sources.
 */
typedef struct hal_sleep_config_t {
    uint16_t size;
    uint16_t version;
    hal_sleep_mode_t mode;
    uint8_t reserved;
    hal_wakeup_source_base_t* wakeup_sources;
} hal_sleep_config_t;

#ifdef __cplusplus
extern "C" {
#endif

int hal_sleep(const hal_sleep_config_t* config, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __SLEEP_HAL_H */