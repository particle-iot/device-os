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

#include "rgbled_hal.h"
#include "system_tick_hal.h"

#include <stddef.h>

// Helper macro used as LEDStatus initializer
#define LED_STATUS(_color, _pattern, _source) \
        { { LED_PATTERN_TYPE_##_pattern, LED_PATTERN_SPEED_NORMAL, RGB_COLOR_##_color } /* LEDPattern */, LED_SOURCE_##_source }

#define LED_COLOR(_status, _color) \
        do { \
            _status.pattern.color = RGB_COLOR_##_color; \
        } while (0)

#define LED_BRIGHTNESS(_status, _level) \
        do { \
            _status.pattern.color = (_status.pattern.color & 0x00ffffff) | ((uint32_t)_level << 24); \
        } while (0)

#define LED_PATTERN(_status, _pattern) \
        do { \
            _status.pattern.type = LED_PATTERN_TYPE_##_pattern; \
        } while (0)

#define LED_SPEED(_status, _speed) \
        do { \
            _status.pattern.speed = LED_PATTERN_SPEED_##_speed; \
        } while (0)

#define LED_ON(_status) \
        do { \
            _status.flags &= ~(uint8_t)LED_STATUS_FLAG_OFF; \
        } while (0)

#define LED_OFF(_status) \
        do { \
            _status.flags |= (uint8_t)LED_STATUS_FLAG_OFF; \
        } while (0)

#define LED_TOGGLE(_status) \
        do { \
            _status.flags ^= (uint8_t)LED_STATUS_FLAG_OFF; \
        } while (0)

#define LED_START(_status, _priority) \
        do { \
            led_status_start(&_status, LED_PRIORITY_##_priority, NULL); \
        } while (0)

#define LED_STOP(_status) \
        do { \
            led_status_stop(&_status, NULL); \
        } while (0)

#define LED_SIGNAL(_signal, _priority) \
        do { \
            led_signal_start(LED_SIGNAL_##_signal, LED_PRIORITY_##_priority, NULL); \
        } while (0)

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
    LED_PATTERN_TYPE_SOLID = 10,
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
    LED_STATUS_FLAG_OFF = 0x01 // Turn LED off
} LEDStatusFlag;

typedef enum {
    LED_THEME_FLAG_USE_AS_DEFAULT = 0x01 // Save signal theme to persistent storage and use it as default theme
} LEDThemeFlag;

struct LEDPattern {
    // TODO: ABI, atomic / volatile
    uint8_t type; // Type (as defined by LEDPatternType enum)
    uint8_t speed; // Speed (as defined by LEDPatternSpeed enum)
    uint32_t color; // Color (0xAARRGGBB)
};

struct LEDStatus {
    // TODO: ABI
    LEDPattern pattern; // Pattern
    uint8_t source; // Source (as defined by LEDSource enum)
    uint8_t flags; // Flags (as defined by LEDStatusFlag enum)
    // Internal data. Caller code should initialize these fields to 0 and never modify them directly again
    LEDStatus* next;
    LEDStatus* prev;
    uint8_t priority;
};

struct LEDTheme {
    // TODO: ABI
    LEDPattern setup_mode;
    LEDPattern network_off;
    LEDPattern network_disconnected;
    LEDPattern network_connecting;
    LEDPattern network_connected;
    LEDPattern cloud_connecting;
    LEDPattern cloud_connected;
};

void led_status_start(LEDStatus* status, int priority, void* reserved);
void led_status_stop(LEDStatus* status, void* reserved);

void led_signal_start(int signal, int priority, void* reserved);
void led_signal_stop(int signal, void* reserved);

void led_theme_set(const LEDTheme* theme, int flags, void* reserved);
void led_theme_get(LEDTheme* theme, void* reserved);

void led_update_enable(void* reserved);
void led_update_disable(void* reserved);

void led_update(system_tick_t ticks, void* reserved);

void led_dump(); // FIXME

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SERVICES_LED_SERVICE_H
