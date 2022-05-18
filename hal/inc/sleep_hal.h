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

#include "hal_platform.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "system_tick_hal.h"
#include "system_defs.h"
#include "interrupts_hal.h"
#include "platforms.h"
#include "assert.h"
#include "usart_hal.h"

#define HAL_SLEEP_VERSION 3

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Stop mode:
 *     What's disabled: The resources occupied by system, e.g. CPU, RGB, external flash etc.
 *     Wakeup sources: Any source that the platform supported.
 *     On-exit: It resumes the disabled resources and continue running.
 * 
 * Ultra-Low power mode:
 *     What's disabled: The resources occupied by system and all other sources those are not featured as wakeup source.
 *     Wakeup sources: Any source that the platform supported
 *     On-exit: It resumes the disabled resources (by restoring peripherals' configuration) and network connection if necessary and continue running.
 * 
 * Hibernate mode:
 *     What's disabled: Most of resources except particular pins and retention RAM
 *     Wakeup sources: Particular pins
 *     On-exit: Reset
 */
typedef enum hal_sleep_mode_t {
    HAL_SLEEP_MODE_NONE = 0,
    HAL_SLEEP_MODE_STOP = 1,
    HAL_SLEEP_MODE_ULTRA_LOW_POWER = 2,
    HAL_SLEEP_MODE_HIBERNATE = 3,
    HAL_SLEEP_MODE_MAX = 0x7F
} hal_sleep_mode_t;

typedef enum hal_wakeup_source_type_t {
    HAL_WAKEUP_SOURCE_TYPE_UNKNOWN = 0,
    HAL_WAKEUP_SOURCE_TYPE_GPIO = 1,
    HAL_WAKEUP_SOURCE_TYPE_ADC = 2,
    HAL_WAKEUP_SOURCE_TYPE_DAC = 3,
    HAL_WAKEUP_SOURCE_TYPE_RTC = 4,
    HAL_WAKEUP_SOURCE_TYPE_LPCOMP = 5,
    HAL_WAKEUP_SOURCE_TYPE_USART = 6,
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

typedef enum hal_sleep_flags_t {
    HAL_SLEEP_FLAG_NONE = 0,
    HAL_SLEEP_FLAG_WAIT_CLOUD = 0x01,
    HAL_SLEEP_FLAG_MAX = 0x7FFFFFFF
} hal_sleep_flags_t;

typedef enum hal_sleep_network_flags_t {
    HAL_SLEEP_NETWORK_FLAG_NONE = 0,
    HAL_SLEEP_NETWORK_FLAG_INACTIVE_STANDBY = 0x01,
    HAL_SLEEP_NETWORK_FLAG_MAX = 0x7FFF
} hal_sleep_network_flags_t;

typedef enum hal_sleep_lpcomp_trig_t {
    HAL_SLEEP_LPCOMP_ABOVE = 0x00,
    HAL_SLEEP_LPCOMP_BELOW = 0x01,
    HAL_SLEEP_LPCOMP_CROSS = 0x02
} hal_sleep_lpcomp_trig_t;

#if PLATFORM_ID > PLATFORM_GCC
static_assert(sizeof(hal_sleep_mode_t) == 1, "length of hal_sleep_mode_t should be 1-bytes aligned.");
static_assert(sizeof(hal_wakeup_source_type_t) == 2, "length of hal_wakeup_source_type_t should be 2-bytes aligned.");
static_assert(sizeof(hal_sleep_flags_t) == 4, "length of hal_sleep_flags_t should be 4");
#endif // PLATFORM_ID > PLATFORM_GCC

/**
 * HAL sleep wakeup source base
 */
typedef struct hal_wakeup_source_base_t {
    uint16_t size;
    uint16_t version;
    hal_wakeup_source_type_t type;
    uint16_t reserved;
    struct hal_wakeup_source_base_t* next;
} hal_wakeup_source_base_t;

/**
 * HAL sleep wakeup source: GPIO
 */
typedef struct hal_wakeup_source_gpio_t {
    hal_wakeup_source_base_t base; // This must come first in order to use casting.
    uint16_t pin;
    InterruptMode mode; // Caution: This might not be 1-byte length, depending on linker options.
    uint8_t reserved;
} hal_wakeup_source_gpio_t;

/**
 * HAL sleep wakeup source: RTC
 */
typedef struct hal_wakeup_source_rtc_t {
    hal_wakeup_source_base_t base; // This must come first in order to use casting.
    system_tick_t ms;
} hal_wakeup_source_rtc_t;

/**
+ * HAL sleep wakeup source: USART
+ */
typedef struct hal_wakeup_source_usart_t {
    hal_wakeup_source_base_t base; // This must come first in order to use casting.
    hal_usart_interface_t serial;
    uint8_t reserved[3];
} hal_wakeup_source_usart_t;

/**
 * HAL sleep wakeup source: network
 */
typedef struct hal_wakeup_source_network_t {
    hal_wakeup_source_base_t base; // This must come first in order to use casting.
    network_interface_index index;
    uint16_t flags; // hal_sleep_network_flags_t
    uint8_t reserved;
} hal_wakeup_source_network_t;

/**
 * HAL sleep wakeup source: low power comparator
 */
typedef struct hal_wakeup_source_lpcomp_t {
    hal_wakeup_source_base_t base; // This must come first in order to use casting.
    uint16_t pin;
    uint16_t voltage; // in mV
    hal_sleep_lpcomp_trig_t trig;
    uint8_t reserved[3];
} hal_wakeup_source_lpcomp_t;

/**
 * HAL sleep configuration: speicify sleep mode and wakeup sources.
 */
typedef struct hal_sleep_config_t {
    uint16_t size;
    uint16_t version;
    hal_sleep_mode_t mode;
    uint8_t reserved;
    uint16_t reserved1;
    uint32_t flags; // hal_sleep_flags_t
    hal_wakeup_source_base_t* wakeup_sources;
} hal_sleep_config_t;

#if PLATFORM_ID > PLATFORM_GCC
static_assert(sizeof(hal_sleep_config_t) == 16, "hal_sleep_config_t incorrect size");
#endif // PLATFORM_ID > PLATFORM_GCC

/**
 * Check if the given sleep configuration is valid or not.
 *
 * @param[in]     config          Sleep configuration that specifies sleep mode, wakeup sources etc.
 *
 * @returns     System error code.
 */
int hal_sleep_validate_config(const hal_sleep_config_t* config, void* reserved);

/**
 * Makes the device enter one of supported sleep modes.
 *
 * @param[in]     config          Sleep configuration that specifies sleep mode, wakeup sources etc.
 * @param[in,out] wakeup_source   Pointer to the wakeup source structure, which is allocated in heap.
 *                                It is caller's responsibility to free this piece of memory.
 *
 * @returns     System error code.
 */
int hal_sleep_enter(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeup_source, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __SLEEP_HAL_H */