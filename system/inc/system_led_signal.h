/*
 * Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.
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

#ifndef SYSTEM_LED_SIGNAL_H
#define SYSTEM_LED_SIGNAL_H

#include "led_service.h"

// Convenience macros allowing to start and stop a signal indication. For example:
//
// LED_SIGNAL_START(NETWORK_CONNECTING, NORMAL); // Starts signal indication with normal priority
// LED_SIGNAL_STOP(NETWORK_CONNECTING); // Stops signal indication
#define LED_SIGNAL_START(_signal, _priority) \
        do { \
            led_start_signal(LED_SIGNAL_##_signal, LED_PRIORITY_VALUE(LED_PRIORITY_##_priority, LED_SOURCE_SYSTEM), 0, NULL); \
        } while (0)

#define LED_SIGNAL_STOP(_signal) \
        do { \
            led_stop_signal(LED_SIGNAL_##_signal, 0, NULL); \
        } while (0)

// Combines system's LED source and priority into a single value as expected by the LED service
#define LED_PRIORITY_VALUE(_priority, _source) \
        ((uint8_t)((((uint8_t)(_priority) & 0x3f) << 2) | ((uint8_t)(_source) & 0x03)))

// Current ABI version number
#define LED_SIGNAL_THEME_VERSION 1

// Number of defined system signals
#define LED_SIGNAL_COUNT 13

#ifdef __cplusplus
extern "C" {
#endif

// System LED signals.
// Note: When adding new signals, make sure LED_SIGNAL_COUNT is updated accordingly
typedef enum LEDSignal {
    LED_SIGNAL_NETWORK_OFF = 0,
    LED_SIGNAL_NETWORK_ON = 1,
    LED_SIGNAL_NETWORK_CONNECTING = 2,
    LED_SIGNAL_NETWORK_DHCP = 3,
    LED_SIGNAL_NETWORK_CONNECTED = 4,
    LED_SIGNAL_CLOUD_CONNECTING = 5,
    LED_SIGNAL_CLOUD_HANDSHAKE = 6,
    LED_SIGNAL_CLOUD_CONNECTED = 7,
    LED_SIGNAL_SAFE_MODE = 8, // Connected to the cloud while in safe mode
    LED_SIGNAL_LISTENING_MODE = 9,
    LED_SIGNAL_DFU_MODE = 10,
    LED_SIGNAL_FIRMWARE_UPDATE = 11,
    LED_SIGNAL_POWER_OFF = 12
} LEDSignal;

// LED signal source
typedef enum LEDSource {
    LED_SOURCE_APPLICATION = 1,
    LED_SOURCE_SYSTEM = 2,
#ifdef PARTICLE_USER_MODULE
    LED_SOURCE_DEFAULT = LED_SOURCE_APPLICATION
#else
    LED_SOURCE_DEFAULT = LED_SOURCE_SYSTEM
#endif
} LEDSource;

// LED signal priority
typedef enum LEDPriority {
    LED_PRIORITY_BACKGROUND = 10,
    LED_PRIORITY_NORMAL = 20,
    LED_PRIORITY_IMPORTANT = 30,
    LED_PRIORITY_CRITICAL = 40
} LEDPriority;

// Predefined LED pattern speed
typedef enum LEDSpeed {
    LED_SPEED_SLOW = 10,
    LED_SPEED_NORMAL = 20,
    LED_SPEED_FAST = 30
} LEDSpeed;

typedef enum LEDSignalFlag {
    LED_SIGNAL_FLAG_SAVE_THEME = 0x01, // Save theme to persistent storage
    LED_SIGNAL_FLAG_DEFAULT_THEME = 0x02, // Initialize theme with factory default settings
    LED_SIGNAL_FLAG_ALL_SIGNALS = 0x04 // Stop all signals
} LEDSignalFlag;

typedef struct LEDSignalThemeData_v1 {
    uint32_t version; // ABI version number. Should be initialized to LED_SIGNAL_THEME_VERSION
    struct { // Signal settings (in order of LEDSignal enum elements)
        uint32_t color; // Color (0x00RRGGBB)
        uint16_t period; // Pattern period in milliseconds
        uint8_t pattern; // Pattern type (as defined by LEDPattern enum)
    } signals[LED_SIGNAL_COUNT];
} LEDSignalThemeData_v1;

typedef LEDSignalThemeData_v1 LEDSignalThemeData;

// Starts signal indication
int led_start_signal(int signal, uint8_t priority, int flags, void* reserved);

// Stops signal indication. Supported flags: LED_SIGNAL_FLAG_ALL_SIGNALS
void led_stop_signal(int signal, int flags, void* reserved);

// Returns 1 if signal indication is started, or 0 otherwise
int led_signal_started(int signal, void* reserved);

// Sets current theme. Supported flags: LED_SIGNAL_FLAG_SAVE_THEME, LED_SIGNAL_FLAG_DEFAULT_THEME
int led_set_signal_theme(const LEDSignalThemeData* theme, int flags, void* reserved);

// Gets current or default theme. Supported flags: LED_SIGNAL_FLAG_DEFAULT_THEME
int led_get_signal_theme(LEDSignalThemeData* theme, int flags, void* reserved);

// Returns immutable LED status data for a signal
const LEDStatusData* led_signal_status(int signal, void* reserved);

// Returns pattern period in milliseconds for a predefined speed value
uint16_t led_pattern_period(int pattern, int speed, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SYSTEM_LED_SIGNAL_H
