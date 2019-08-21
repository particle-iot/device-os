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

#pragma once

/**
 * Network interfaces.
 */
typedef enum network_interface_index {
    NETWORK_INTERFACE_ALL = 0,
    NETWORK_INTERFACE_LOOPBACK = 1,
    NETWORK_INTERFACE_MESH = 2,
    NETWORK_INTERFACE_ETHERNET = 3,
    NETWORK_INTERFACE_CELLULAR = 4,
    NETWORK_INTERFACE_WIFI_STA = 4,
    NETWORK_INTERFACE_WIFI_AP = 5
} network_interface_index;

/**
 * System reset reason.
 *
 * @note These reason codes are converted to string identifiers and get published via the
 * spark/device/last_reset event. When adding a new reason code, make sure to update the
 * mappings accordingly.
 */
typedef enum system_reset_reason {
    RESET_REASON_NONE = 0, ///< Invalid reason code.
    RESET_REASON_UNKNOWN = 10, ///< Unspecified reason.
    // Hardware
    RESET_REASON_PIN_RESET = 20, ///< Reset from the reset pin.
    RESET_REASON_POWER_MANAGEMENT = 30, ///< Low-power management reset.
    RESET_REASON_POWER_DOWN = 40, ///< Power-down reset.
    RESET_REASON_POWER_BROWNOUT = 50, ///< Brownout reset.
    RESET_REASON_WATCHDOG = 60, ///< Watchdog reset.
    // Software
    RESET_REASON_UPDATE = 70, ///< Reset to apply firmware update.
    RESET_REASON_UPDATE_ERROR = 80, ///< Generic firmware update error (deprecated).
    RESET_REASON_RECOVERY = 90, ///< Recovery reset.
    RESET_REASON_UPDATE_TIMEOUT = RESET_REASON_RECOVERY, ///< Firmware update timeout (deprecated).
    RESET_REASON_FACTORY_RESET = 100, ///< Factory reset requested.
    RESET_REASON_SAFE_MODE = 110, ///< Safe mode requested.
    RESET_REASON_DFU_MODE = 120, ///< DFU mode requested.
    RESET_REASON_PANIC = 130, ///< System panic.
    RESET_REASON_USER = 140 ///< User-requested reset.
} system_reset_reason;
