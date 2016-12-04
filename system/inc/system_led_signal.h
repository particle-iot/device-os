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

#define LED_SIGNAL_START(_signal, _priority) \
        do { \
            led_start_signal(LED_SIGNAL_##_signal, LED_PRIORITY_VALUE(LED_PRIORITY_##_priority, LED_SOURCE_SYSTEM), NULL); \
        } while (0)

#define LED_SIGNAL_STOP(_signal) \
        do { \
            led_stop_signal(LED_SIGNAL_##_signal, NULL) \
        } while (0)

#define LED_PRIORITY_VALUE(_priority, _source) \
        ((uint8_t)((((uint8_t)(_priority) & 0x3f) << 2) | ((uint8_t)(_source) & 0x03)))

#define LED_SIGNAL_COUNT 13
#define LED_PALETTE_COLOR_COUNT 8

#define LED_THEME_DATA_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LED_SIGNAL_NETWORK_OFF = 0,
    LED_SIGNAL_NETWORK_ON = 1,
    LED_SIGNAL_NETWORK_CONNECTING = 2,
    LED_SIGNAL_NETWORK_DHCP = 3,
    LED_SIGNAL_NETWORK_CONNECTED = 4,
    LED_SIGNAL_CLOUD_CONNECTING = 5,
    LED_SIGNAL_CLOUD_HANDSHAKE = 6,
    LED_SIGNAL_CLOUD_CONNECTED = 7,
    LED_SIGNAL_LISTENING_MODE = 8,
    LED_SIGNAL_SAFE_MODE = 9,
    LED_SIGNAL_DFU_MODE = 10,
    LED_SIGNAL_FIRMWARE_UPDATE = 11,
    LED_SIGNAL_POWER_OFF = 12
} LEDSignal;

typedef enum {
    LED_SOURCE_APPLICATION = 1,
    LED_SOURCE_SYSTEM = 2,
#ifdef PARTICLE_USER_MODULE
    LED_SOURCE_DEFAULT = LED_SOURCE_APPLICATION
#else
    LED_SOURCE_DEFAULT = LED_SOURCE_SYSTEM
#endif
} LEDSource;

typedef enum {
    LED_PRIORITY_BACKGROUND = 10,
    LED_PRIORITY_NORMAL = 20,
    LED_PRIORITY_IMPORTANT = 30,
    LED_PRIORITY_CRITICAL = 40
} LEDPriority;

typedef struct {
    uint8_t color; // Color index (0 to LED_PALETTE_COLOR_COUNT - 1)
    uint8_t pattern; // Pattern type (as defined by LEDPattern enum)
    uint8_t speed; // Pattern speed (as defined by LEDSpeed enum)
} LEDThemeSignalData_v1;

typedef struct {
    uint32_t version; // ABI version number (see LED_THEME_DATA_VERSION)
    uint32_t palette[LED_PALETTE_COLOR_COUNT]; // Palette colors: 0x00RRGGBB, ...
    LEDThemeSignalData_v1 signals[LED_SIGNAL_COUNT]; // Signal styles (in order as LEDSignal elements)
} LEDThemeData_v1;

typedef enum {
    LED_THEME_FLAG_DEFAULT = 0x01 // Initialize theme with factory default parameters
} LEDThemeFlag;

typedef LEDThemeSignalData_v1 LEDThemeSignalData;
typedef LEDThemeData_v1 LEDThemeData;

int led_start_signal(int signal, uint8_t priority, void* reserved);
void led_stop_signal(int signal, void* reserved);
int led_is_signal_started(int signal, void* reserved);

int led_set_signal_theme(const LEDThemeData* theme, int flags, void* reserved);
int led_get_signal_theme(LEDThemeData* theme, int flags, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SYSTEM_LED_SIGNAL_H
