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

#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL

#include "i2c_hal.h"

#if HAL_PLATFORM_AM18X5
#include "exrtc_hal_am18x5.h"
#endif // HAL_PLATFORM_AM18X5

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define HAL_EXRTC_API_VERSION (3)

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
    HAL_EXRTC_CONFIG_USE_FOR_RTC = 0x01,
} hal_exrtc_config_flag_t;

typedef enum hal_exrtc_capability_t {
    HAL_EXRTC_CAPS_NONE = 0,
    HAL_EXRTC_CAPS_POWER_GATE = 0x01,
    HAL_EXRTC_CAPS_CLOCK_SOURCE = 0x02,
    HAL_EXRTC_CAPS_CLOCK_OUTPUT = 0x04,
    HAL_EXRTC_CAPS_AUTO_CALIBRATION = 0x08,
    HAL_EXRTC_CAPS_CLOCK_SOURCE_INTERNAL_ON_BATTERY = 0x10,
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
} __attribute__((packed)) hal_i2c_device_t;

typedef struct hal_exrtc_device_t {
    uint16_t version;
    uint16_t size;

    hal_exrtc_type_t type;
    uint32_t flags;

    uint32_t reserved[4];
    void* reserved1;

    uint8_t transport;
    union {
        hal_i2c_device_t i2c;
    };
    uint32_t reserved[4];
    // Do not add any more fields here unless version
    // is changed and version change is correctly handled
} __attribute__((packed)) hal_exrtc_device_t;

typedef struct hal_exrtc_vendor_config_t {
    uint16_t version;
    uint16_t size;

    hal_exrtc_type_t type;
} __attribute__((packed)) hal_exrtc_vendor_config_t;

typedef struct hal_exrtc_config_t {
    uint16_t version;
    uint16_t size;

    uint32_t flags;
    uint32_t reserved[4];
} __attribute__((packed)) hal_exrtc_config_t;

typedef struct hal_exrtc_status_t {
    uint16_t version;
    uint16_t size;

    uint32_t status;
    uint32_t capabilities;

    hal_exrtc_type_t type;

    hal_exrtc_clock_source_t clock_source;

    uint32_t reserved[4]; 
} __attribute__((packed)) hal_exrtc_status_t;

typedef struct hal_exrtc_binding_t {
    uint16_t version;
    uint16_t size;

    hal_exrtc_device_t* device;
    // Optional
    hal_exrtc_config_t* config;
    hal_exrtc_vendor_config_t* vendor;

}  __attribute__((packed)) hal_exrtc_binding_t;

typedef void (*hal_exrtc_event_handler_t)(uint32_t events, void* extra, void* context);

int hal_exrtc_bind(hal_exrtc_instance_t instance, const hal_exrtc_binding_t* binding, void* reserved);
int hal_exrtc_get_device(hal_exrtc_instance_t instance, hal_exrtc_device_t* device, void* reserved);
int hal_exrtc_unbind(hal_exrtc_instance_t instance, void* reserved);

int hal_exrtc_get_status(hal_exrtc_instance_t instance, hal_exrtc_status_t* status, void* reserved, void* reserved1);

int hal_exrtc_set_config(hal_exrtc_instance_t instance, const hal_exrtc_config_t* config, const hal_exrtc_vendor_config_t* vendor, void* reserved);
int hal_exrtc_get_config(hal_exrtc_instance_t instance, hal_exrtc_config_t* config, hal_exrtc_vendor_config_t* vendor, void* reserved);

void* hal_exrtc_event_handler_add(hal_exrtc_instance_instance instance, hal_exrtc_event_handler_t handler, void* context, void* reserved);

int hal_exrtc_command(hal_exrtc_instance_t instance, hal_exrtc_command_t cmd, void* arg, void* reserved);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
