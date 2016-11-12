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

#ifndef SERVICES_LED_SERVICE_H
#define SERVICES_LED_SERVICE_H

#include "rgbled.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LED_SOURCE_SYSTEM = 0,
    LED_SOURCE_APPLICATION = 1
} LEDSource;

typedef enum {
    LED_PRIORITY_BACKGROUND = 0,
    LED_PRIORITY_NORMAL = 1,
    LED_PRIORITY_IMPORTANT = 2,
    LED_PRIORITY_CRITICAL = 3
} LEDPriority;

typedef enum {
    LED_PATTERN_TYPE_STEADY = 10,
    LED_PATTERN_TYPE_BLINK = 20,
    LED_PATTERN_TYPE_FADE = 30
} LEDPatternType;

typedef enum {
    LED_PATTERN_SPEED_SLOW = 10,
    LED_PATTERN_SPEED_NORMAL = 20,
    LED_PATTERN_SPEED_FAST = 30
} LEDPatternSpeed;

typedef enum {
    LED_SIGNAL_SETUP_MODE = 10,
    LED_SIGNAL_NETWORK_OFF = 20,
    LED_SIGNAL_NETWORK_DISCONNECTED = 30,
    LED_SIGNAL_NETWORK_CONNECTING = 40,
    LED_SIGNAL_NETWORK_CONNECTED = 50,
    LED_SIGNAL_CLOUD_CONNECTING = 60,
    LED_SIGNAL_CLOUD_HANDSHAKE = 70,
    LED_SIGNAL_CLOUD_CONNECTED = 80
    // TODO
} LEDSignal;

typedef enum {
    LED_FLAG_USE_AS_DEFAULT = 0x01 // Save signal theme to persistent storage
} LEDFlag;

struct LEDPattern {
    // TODO: ABI
    uint8_t type; // Type (as defined by LEDPatternType enum)
    uint8_t speed; // Speed (as defined by LEDPatternSpeed enum)
    uint32_t color; // Color (0x00RRGGBB)
};

struct LEDState {
    // TODO: ABI
    LEDPattern pattern; // Pattern
    // Internal data. Caller code should initialize these fields to 0 and never modify them directly since then
    LEDState* next;
    LEDState* prev;
    uint8_t source;
    uint8_t priority;
};

struct LEDSignalTheme {
    // TODO: ABI
    LEDPattern setup_mode;
    LEDPattern network_off;
    LEDPattern network_disconnected;
    LEDPattern network_connecting;
    LEDPattern network_connected;
    LEDPattern cloud_connecting;
    LEDPattern cloud_connected;
};

void led_enter_state(LEDState* state, int source, int priority, void* reserved);
void led_leave_state(LEDState* state, void* reserved);

void led_start_signal(int signal, int priority, void* reserved);
void led_stop_signal(int signal, void* reserved);

void led_set_signal_theme(const LEDSignalTheme* theme, int flags, void* reserved);
void led_get_signal_theme(LEDSignalTheme* theme, void* reserved);

void led_enable_updates(void* reserved);
void led_disable_updates(void* reserved);

void led_update(system_tick_t time, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SERVICES_LED_SERVICE_H
