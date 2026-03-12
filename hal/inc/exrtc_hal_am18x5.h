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

#if HAL_PLATFORM_AM18X5

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct hal_exrtc_vendor_config_am18x5_t {
    hal_exrtc_vendor_config_t base;

    union {
        struct {
            uint8_t xtal_calibration_set : 1;
        };
        uint32_t flags;
    };
    int8_t xtal_calibration;
} hal_exrtc_vendor_config_am18x5_t;

#define HAL_EXRTC_TYPE_AM18X5_DEFAULT_ADDRESS (0x69)

typedef struct am18x5_manufacturing_config_t {
    uint16_t version;
    uint16_t size;
    uint8_t default_rtc;
    uint8_t wdi_pin;
    uint8_t int_pin;
    hal_i2c_interface_t i2c_if;
    uint8_t rc_fallback;
    uint8_t rc_on_battery;
    uint8_t osc_src;
    int8_t osc_cal_xt;
    uint8_t clk_out_en;
    uint8_t clk_out_freq;
    uint8_t auto_calibration;
    uint32_t mfg_magic;
    int8_t mfg_osc_cal_xt; // Read only
    uint8_t reserved[12];
} __attribute__((packed)) am18x5_manufacturing_config_t;

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HAL_PLATFORM_AM18X5
