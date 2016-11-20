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

#include "system_tick_hal.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Pattern type
typedef enum {
    LED_PATTERN_TYPE_INVALID = 0,
    LED_PATTERN_TYPE_SOLID = 10,
    LED_PATTERN_TYPE_BLINK = 20,
    LED_PATTERN_TYPE_FADE = 30
} LEDPatternType;

// Pattern speed
typedef enum {
    LED_PATTERN_SPEED_SLOW = 10,
    LED_PATTERN_SPEED_NORMAL = 20,
    LED_PATTERN_SPEED_FAST = 30
} LEDPatternSpeed;

// Status flags
typedef enum {
    LED_STATUS_FLAG_ACTIVE = 0x01, // LED status is active (do not set this flag manually)
    LED_STATUS_FLAG_OFF = 0x02 // LED is turned off
} LEDStatusFlag;

typedef struct LEDStatusData {
    size_t size; // Size of this structure
    struct LEDStatusData* next; // Internal field. Should be initialized to 0
    struct LEDStatusData* prev; // Ditto
    volatile uint32_t color; // Color (0x00RRGGBB)
    volatile uint8_t value; // Brightness (0 - 255)
    volatile uint8_t pattern; // Pattern type (as defined by LEDPatternType enum)
    volatile uint8_t speed; // Pattern speed (as defined by LEDPatternSpeed enum)
    volatile uint8_t flags; // Flags (as defined by LEDStatusFlag enum)
    uint8_t priority; // Priority (0 - 255)
} LEDStatusData;

// Starts / stops LED status indication
void led_set_status_active(LEDStatusData* status, bool active, void* reserved);

// Allows to temporary disable updating of LED color by led_update() function
void led_set_updates_enabled(bool enabled, void* reserved);

// Updates LED color according to a number of ticks passed since previous update. This function needs
// to be called periodically
void led_update(system_tick_t ticks, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SERVICES_LED_SERVICE_H
