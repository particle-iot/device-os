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
