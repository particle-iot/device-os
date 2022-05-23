/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include "application.h"
#include "unit-test/unit-test.h"

test(UART_01_PinConfigurationNotAffectedWhenEndCalledMultipleTimes) {
    Serial1.begin(115200, SERIAL_8N1);
    assertTrue(Serial1.isEnabled());
    Serial1.end();
    assertFalse(Serial1.isEnabled());

    pinMode(TX, INPUT_PULLDOWN);
    pinMode(RX, INPUT_PULLDOWN);

    assertEqual(getPinMode(TX), INPUT_PULLDOWN);
    assertEqual(getPinMode(RX), INPUT_PULLDOWN);

    Serial1.end();
    assertEqual(getPinMode(TX), INPUT_PULLDOWN);
    assertEqual(getPinMode(RX), INPUT_PULLDOWN);

    pinMode(TX, INPUT);
    pinMode(RX, INPUT);
}
