/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#pragma once

#include "hal_platform.h"

#if HAL_PLATFORM_EXTERNAL_RTC

#include "i2c_hal.h"
#include "interrupts_hal.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define HAL_EXRTC_API_VERSION_BASE (3)
#define HAL_EXRTC_API_VERSION (3)
#define HAL_EXRTC_MAX_ID_SIZE (18)

typedef enum hal_exrtc_instance_t {
    HAL_EXRTC_INSTANCE_1 = 0,
    HAL_EXRTC_INSTANCE_DEFAULT = HAL_EXRTC_INSTANCE_1
} hal_exrtc_instance_t;

typedef enum hal_exrtc_type_t {
    HAL_EXRTC_TYPE_NONE = 0,
    HAL_EXRTC_TYPE_UNKNOWN = 1,
#if HAL_PLATFORM_AM18X5
    HAL_EXRTC_TYPE_AM18X5 = 0x00001805,
#endif // HAL_PLATFORM_AM18X5
    HAL_EXRTC_TYPE_MAX = 0xffffffff
} hal_exrtc_type_t;

typedef enum hal_exrtc_transport_t {
    // XXX: interface sounds confusing
    HAL_EXRTC_TRANSPORT_NONE = 0,
    HAL_EXRTC_TRANSPORT_I2C = 1
} hal_exrtc_transport_t;

typedef enum hal_exrtc_status_flag_t {
    HAL_EXRTC_STATUS_NONE = 0,
    HAL_EXRTC_STATUS_BUILT_IN = 0x01,
    HAL_EXRTC_STATUS_BOUND = 0x02,
    HAL_EXRTC_STATUS_PRESENT = 0x04,
    HAL_EXRTC_STATUS_READY = 0x08
} hal_exrtc_status_flag_t;

typedef enum hal_exrtc_config_flag_t {
    HAL_EXRTC_CONFIG_NONE = 0,
    HAL_EXRTC_CONFIG_USE_AS_MAIN_RTC = 0x01,
    HAL_EXRTC_CONFIG_DEFAULT_PLATFORM_CONFIG = 0x02,
    HAL_EXRTC_CONFIG_SLEEP_EXTI_CHECK = 0x04
} hal_exrtc_config_flag_t;

typedef enum hal_exrtc_capability_t {
    HAL_EXRTC_CAPS_NONE = 0,
    HAL_EXRTC_CAPS_POWER_GATE = 0x01,
    HAL_EXRTC_CAPS_CLOCK_SOURCE = 0x02,
    HAL_EXRTC_CAPS_CLOCK_OUTPUT = 0x04,
    HAL_EXRTC_CAPS_AUTO_CALIBRATION = 0x08,
    HAL_EXRTC_CAPS_AUTO_CLOCK_SOURCE_INTERNAL_ON_BATTERY = 0x10,
    HAL_EXRTC_CAPS_AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL = 0x20,
    HAL_EXRTC_CAPS_SLEEP = 0x40,
    HAL_EXRTC_CAPS_EXTI = 0x80,
    HAL_EXRTC_CAPS_EXTI_LEVEL_TRIGGER = 0x100,
    HAL_EXRTC_CAPS_WATCHDOG = 0x200,
} hal_exrtc_capability_t;

typedef enum hal_exrtc_event_t {
    HAL_EXRTC_EVENT_NONE = 0,
    HAL_EXRTC_EVENT_CALIBRATION_FAILURE = 0x01,
    HAL_EXRTC_EVENT_CLOCK_SOURCE_EXTERNAL_FAILURE = 0x02
} hal_exrtc_event_t;

typedef enum hal_exrtc_clock_source_t {
    HAL_EXRTC_CLOCK_SOURCE_NONE = 0,
    HAL_EXRTC_CLOCK_SOURCE_INTERNAL = 1,
    HAL_EXRTC_CLOCK_SOURCE_EXTERNAL = 2,
} hal_exrtc_clock_source_t;

// Move to i2c hal
typedef struct hal_i2c_device_t {
    hal_i2c_interface_t interface;
    uint8_t address;
    hal_pin_t pin_int;
    hal_pin_t pins[4];
    uint32_t reserved[4];
} hal_i2c_device_t;

typedef struct hal_exrtc_device_t {
    uint16_t version;
    uint16_t size;

    hal_exrtc_type_t type;
    uint32_t flags; // reserved for now

    uint32_t reserved[4];

    hal_exrtc_transport_t transport;
    union {
        hal_i2c_device_t i2c;
    };
    uint32_t reserved1[4];
    // Do not add any more fields here unless version
    // is changed and version change is correctly handled
} hal_exrtc_device_t;

typedef struct hal_exrtc_vendor_config_t {
    uint16_t version;
    uint16_t size;

    hal_exrtc_type_t type;
    uint8_t reserved[3];
    // This is a base struct, do not add any more fields to it
} hal_exrtc_vendor_config_t;

typedef struct hal_exrtc_config_t {
    uint16_t version;
    uint16_t size;

    uint32_t flags; //  hal_exrtc_config_flag_t
    uint32_t caps_enable; // hal_exrtc_capability_t

    hal_exrtc_clock_source_t clock_source;
    uint32_t clock_output_frequency;

    uint32_t reserved[3];
} hal_exrtc_config_t;

typedef struct hal_exrtc_status_t {
    uint16_t version;
    uint16_t size;

    hal_exrtc_type_t type;

    uint32_t status; // hal_exrtc_status_flag_t
    uint32_t caps_supported; // hal_exrtc_capability_t
    uint32_t caps_optional; // hal_exrtc_capability_t
    uint32_t caps_enabled; // hal_exrtc_capability_t
    uint32_t reserved;

    hal_exrtc_clock_source_t clock_source;
    int32_t xtal_calibration;

    uint32_t reserved1[3];
} hal_exrtc_status_t;

typedef struct hal_exrtc_binding_t {
    uint16_t version;
    uint16_t size;

    hal_exrtc_device_t* device;
    // Optional
    hal_exrtc_config_t* config;
    hal_exrtc_vendor_config_t* vendor;
}  hal_exrtc_binding_t;

typedef enum hal_exrtc_command_t {
    HAL_EXRTC_COMMAND_NONE = 0,
    HAL_EXRTC_COMMAND_WRITE_MFG_XTAL_CALIBRATION = 1,
    HAL_EXRTC_COMMAND_SLEEP = 2,
    HAL_EXRTC_COMMAND_GET_ID = 3,
    HAL_EXRTC_COMMAND_READ_MFG_XTAL_CALIBRATION = 4
} hal_exrtc_command_t;

typedef struct hal_exrtc_calibration_data_t {
    uint16_t version;
    uint16_t size;

    int32_t value;
    uint32_t reserved[4];
} hal_exrtc_calibration_data_t;

typedef struct hal_exrtc_sleep_config_t {
    uint16_t version;
    uint16_t size;

    InterruptMode exti_mode;
    system_tick_t duration;
    uint8_t reserved[8];
} hal_exrtc_sleep_config_t;

typedef void (*hal_exrtc_event_handler_t)(uint32_t events, void* extra, void* context);
typedef void (*hal_exrtc_event_cleanup_handler_t)(void* context);

int hal_exrtc_bind(hal_exrtc_instance_t instance, const hal_exrtc_binding_t* binding, void* reserved);
int hal_exrtc_get_device(hal_exrtc_instance_t instance, hal_exrtc_device_t* device, void* reserved);
int hal_exrtc_unbind(hal_exrtc_instance_t instance, void* reserved);

int hal_exrtc_get_status(hal_exrtc_instance_t instance, hal_exrtc_status_t* status, void* reserved, void* reserved1);

int hal_exrtc_set_config(hal_exrtc_instance_t instance, const hal_exrtc_config_t* config, const hal_exrtc_vendor_config_t* vendor, void* reserved);
int hal_exrtc_get_config(hal_exrtc_instance_t instance, hal_exrtc_config_t* config, hal_exrtc_vendor_config_t* vendor, void* reserved);

void* hal_exrtc_event_handler_add(hal_exrtc_instance_t instance, hal_exrtc_event_handler_t handler, void* context, hal_exrtc_event_cleanup_handler_t cleanup, void* reserved);
int hal_exrtc_event_handler_del(hal_exrtc_instance_t instance, void* cookie, void* reserved);

int hal_exrtc_command(hal_exrtc_instance_t instance, hal_exrtc_command_t cmd, void* arg, uint32_t arg1, void* reserved);

#ifdef __cplusplus
}
#endif // __cplusplus

#if HAL_PLATFORM_AM18X5
#include "exrtc_hal_am18x5.h"
#endif // HAL_PLATFORM_AM18X5

#endif // HAL_PLATFORM_EXTERNAL_RTC
