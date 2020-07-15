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
    // NETWORK_INTERFACE_MESH = 2, // Deprecated
    NETWORK_INTERFACE_ETHERNET = 3,
    NETWORK_INTERFACE_CELLULAR = 4,
    NETWORK_INTERFACE_WIFI_STA = 4,
    NETWORK_INTERFACE_WIFI_AP = 5
} network_interface_index;

/**
 * System reset reason.
 */
// When adding a new reason code here, make sure to update the system's resetReasonString()
// function accordingly
typedef enum System_Reset_Reason {
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
    RESET_REASON_UPDATE_TIMEOUT = 90, ///< Firmware update timeout.
    RESET_REASON_FACTORY_RESET = 100, ///< Factory reset requested.
    RESET_REASON_SAFE_MODE = 110, ///< Safe mode requested.
    RESET_REASON_DFU_MODE = 120, ///< DFU mode requested.
    RESET_REASON_PANIC = 130, ///< System panic.
    RESET_REASON_USER = 140 ///< User-requested reset.
} System_Reset_Reason;

/**
 * Cloud disconnection reason.
 */
typedef enum cloud_disconnect_reason {
    CLOUD_DISCONNECT_REASON_NONE = 0, ///< Invalid reason code.
    CLOUD_DISCONNECT_REASON_ERROR = 1, ///< Disconnected due to an error.
    CLOUD_DISCONNECT_REASON_USER = 2, ///< Disconnected at the user's request.
    CLOUD_DISCONNECT_REASON_NETWORK_DISCONNECT = 3, ///< Disconnected due to a network disconnection.
    CLOUD_DISCONNECT_REASON_LISTENING = 4, ///< Disconnected to enter the listening mode.
    CLOUD_DISCONNECT_REASON_SYSTEM_RESET = 5, ///< Disconnected due to a system reset.
    CLOUD_DISCONNECT_REASON_SLEEP = 6, ///< Disconnected to enter a sleep mode.
    CLOUD_DISCONNECT_REASON_UNKNOWN = 7 ///< Unspecified disconnection reason.
} cloud_disconnect_reason;

/**
 * Network disconnection reason.
 */
typedef enum network_disconnect_reason {
    NETWORK_DISCONNECT_REASON_NONE = 0, ///< Invalid reason code.
    NETWORK_DISCONNECT_REASON_ERROR = 1, ///< Disconnected due to an error.
    NETWORK_DISCONNECT_REASON_USER = 2, ///< Disconnected at the user's request.
    NETWORK_DISCONNECT_REASON_NETWORK_OFF = 3, ///< Disconnected due to a network shutdown (deprecated).
    NETWORK_DISCONNECT_REASON_LISTENING = 4, ///< Disconnected to enter the listening mode.
    NETWORK_DISCONNECT_REASON_SLEEP = 5, ///< Disconnected to enter a sleep mode.
    NETWORK_DISCONNECT_REASON_RESET = 6, ///< Disconnected to recover from a cloud connection error.
    NETWORK_DISCONNECT_REASON_UNKNOWN = 7 ///< Unspecified disconnection reason.
} network_disconnect_reason;

#ifdef __cplusplus

#include "enumflags.h"

namespace particle {

/**
 * Firmware update flags.
 */
enum class FirmwareUpdateFlag {
    DISCARD_DATA = 0x01, ///< Discard any previously received firmware data.
    NON_RESUMABLE = 0x02, ///< Indicates that the update cannot be resumed.
    VALIDATE_ONLY = 0x04, ///< Validate the parameters but do not start/finish the update.
    CANCEL = 0x08 ///< Cancel the update.
};

typedef EnumFlags<FirmwareUpdateFlag> FirmwareUpdateFlags;

ENABLE_ENUM_CLASS_BITWISE(FirmwareUpdateFlag);

} // namespace particle

#endif // defined(__cplusplus)
