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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef POWER_HAL_H
#define POWER_HAL_H

#include <stdint.h>
#include <stddef.h>
#include "static_assert.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum hal_power_config_flags {
    // For platforms where PMIC is not present by default on-board
    // optionally enables its presence detection in runtime
    HAL_POWER_PMIC_DETECTION = 0x01,
    // Next 7 flags reserved to keep DCT format compatibility
    HAL_POWER_FLAG_RESERVED1 = 0x02,
    HAL_POWER_FLAG_RESERVED2 = 0x04,
    HAL_POWER_FLAG_RESERVED3 = 0x08,
    HAL_POWER_FLAG_RESERVED4 = 0x10,
    HAL_POWER_FLAG_RESERVED5 = 0x20,
    HAL_POWER_FLAG_RESERVED6 = 0x40,
    HAL_POWER_FLAG_RESERVED7 = 0x80,
    // End reserved, add new flags here

    // Disables power management leaving the control of PMIC
    // completely to the application
    HAL_POWER_MANAGEMENT_DISABLE = 0x100,
    HAL_POWER_USE_VIN_SETTINGS_WITH_USB_HOST = 0x200,

    // Disables power management control over enabling charging on PMIC
    HAL_POWER_CHARGE_STATE_DISABLE = 0x400,

    HAL_POWER_FLAG_MAX = 0x7fffffff
} hal_power_config_flags;

typedef struct hal_power_config {
    // Flags are the first field to maintain DCT format compatibility for SYSTEM_FLAG_PM_DETECTION
    uint32_t flags; // hal_power_config_flags
    uint8_t version;
    uint8_t size;
    uint16_t vin_min_voltage; // min voltage the VIN PSU can provide
    uint16_t vin_max_current; // max current the VIN PSU can provide
    uint16_t charge_current; // charge current
    uint16_t termination_voltage; // termination voltage
    uint8_t soc_bits; // bits precision for SoC calculation (18 (default) or 19)
    uint8_t reserved2;
    uint32_t reserved3[4];
} hal_power_config;
static_assert(sizeof(hal_power_config) == 32, "hal_power_config size changed");

int hal_power_load_config(hal_power_config* conf, void* reserved);
int hal_power_store_config(const hal_power_config* conf, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif /* POWER_HAL_H */
