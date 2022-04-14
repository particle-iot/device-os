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
typedef enum LEDPattern {
    LED_PATTERN_INVALID = 0,
    LED_PATTERN_SOLID = 1,
    LED_PATTERN_BLINK = 2,
    LED_PATTERN_FADE = 3,
    LED_PATTERN_CUSTOM = 15 // Should be last element in this enum
} LEDPattern;

// Status flags
typedef enum LEDStatusFlag {
    LED_STATUS_FLAG_ACTIVE = 0x01, // LED status is active (do not modify this flag directly)
    LED_STATUS_FLAG_OFF = 0x02 // LED is turned off
} LEDStatusFlag;

// Status data
typedef struct LEDStatusData {
    size_t size; // Size of this structure
    struct LEDStatusData* next; // Internal field. Should be initialized to NULL
    struct LEDStatusData* prev; // ditto
    volatile uint8_t priority; // Priority (0 - 255)
    volatile uint8_t pattern; // Pattern type (as defined by LEDPattern enum)
    volatile uint8_t flags; // Flags (as defined by LEDStatusFlag enum)
    volatile uint32_t color; // Color (0x00RRGGBB)
    union { // Pattern parameters
        struct { // Predefined pattern (LEDStatusData::pattern != LED_PATTERN_CUSTOM)
            volatile uint16_t period; // Pattern period in milliseconds
        };
        struct { // Custom pattern (LEDStatusData::pattern == LED_PATTERN_CUSTOM)
            void(*callback)(system_tick_t, void*); // User callback
            void* data; // Callback data
        };
    };
} LEDStatusData;

// Starts/stops LED status indication
void led_set_status_active(LEDStatusData* status, int active, void* reserved);

// Enables/disables updating of LED color by led_update() function
void led_set_update_enabled(int enabled, void* reserved);

// Returns 1 if updating of LED color by led_update() function is enabled, or 0 otherwise
int led_update_enabled(void* reserved);

// Updates LED color according to a number of ticks passed since previous update. This function needs
// to be called periodically.
//
// TODO: If `status` argument is not null, LED will be updated only if specified status has a highest
// priority among other active statuses. This can be used to implement a status indication that
// requires real time LED updates
void led_update(system_tick_t ticks, LEDStatusData* status, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SERVICES_LED_SERVICE_H
