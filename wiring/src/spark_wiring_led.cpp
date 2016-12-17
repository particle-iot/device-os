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

#include "spark_wiring_led.h"

// particle::LEDStatus
particle::LEDStatus::LEDStatus(uint32_t color, LEDPattern pattern, uint16_t period, LEDPriority priority, LEDSource source) {
    d_.size = sizeof(LEDStatusData);
    d_.next = nullptr;
    d_.prev = nullptr;
    d_.priority = LED_PRIORITY_VALUE(priority, source); // Combine source and priority into single value
    d_.pattern = (pattern == LED_PATTERN_CUSTOM ? LED_PATTERN_SOLID : pattern); // Sanity check
    d_.flags = 0;
    d_.color = color;
    d_.period = period;
}

// particle::LEDCustomStatus
particle::LEDCustomStatus::LEDCustomStatus(uint32_t color, LEDPriority priority, LEDSource source) {
    d_.size = sizeof(LEDStatusData);
    d_.next = nullptr;
    d_.prev = nullptr;
    d_.priority = LED_PRIORITY_VALUE(priority, source);
    d_.pattern = LED_PATTERN_CUSTOM;
    d_.flags = 0;
    d_.color = color;
    d_.callback = updateCallback; // User callback
    d_.data = this; // Callback data
}

// particle::detail::*
uint16_t particle::detail::patternPeriod(LEDPattern pattern, LEDSpeed speed) {
    switch (pattern) {
    case LED_PATTERN_BLINK:
        // Blinking LED
        if (speed == LED_SPEED_NORMAL) {
            return 500; // Normal
        } else if (speed > LED_SPEED_NORMAL) {
            return 200; // Fast
        } else {
            return 1000; // Slow
        }
    case LED_PATTERN_FADE:
        // "Breathing" LED
        if (speed == LED_SPEED_NORMAL) {
            return 4000; // Normal
        } else if (speed > LED_SPEED_NORMAL) {
            return 1000; // Fast
        } else {
            return 7000; // Slow
        }
    default:
        return 0; // Not applicable
    }
}
