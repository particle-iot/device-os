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
#include "Serial2/Serial2.h"
#include "Serial3/Serial3.h"
#include "Serial4/Serial4.h"
#include "Serial5/Serial5.h"

#if HAL_PLATFORM_GEN == 2

namespace {

uint32_t getPinAf(pin_t pin) {
    auto pinmap = HAL_Pin_Map();
    auto gpiox = pinmap[pin].gpio_peripheral;
    auto pinSource = pinmap[pin].gpio_pin_source;
    auto afr = gpiox->AFR[pinSource >> 0x03];
    return (afr >> ((pinSource & 0x07) * 4)) & 0xF;
}

} // anomymous

test(UART_00_HardwareFlowControlCannotBeEnabledOnUnsupportedPeripheral) {
    // On Gen 2 not supported on Serial1 and Serial4/5
    Serial1.end();
    Serial1.begin(115200, SERIAL_8N1 | SERIAL_FLOW_CONTROL_RTS_CTS);
    assertFalse(Serial1.isEnabled());

    Serial1.begin(115200, SERIAL_8N1);
    assertTrue(Serial1.isEnabled());
    Serial1.end();

#if Wiring_Serial4
    Serial4.end();
    Serial4.begin(115200, SERIAL_8N1 | SERIAL_FLOW_CONTROL_RTS_CTS);
    assertFalse(Serial4.isEnabled());

    Serial4.begin(115200, SERIAL_8N1);
    assertTrue(Serial4.isEnabled());
    Serial4.end();
#endif // Wiring_Serial4

#if Wiring_Serial5
    Serial5.end();
    Serial5.begin(115200, SERIAL_8N1 | SERIAL_FLOW_CONTROL_RTS_CTS);
    assertFalse(Serial5.isEnabled());

    Serial5.begin(115200, SERIAL_8N1);
    assertTrue(Serial5.isEnabled());
    Serial5.end();
#endif // Wiring_Serial5
}

test(UART_01_D0ConfigurationIsIntactWhenSerial1IsDeinitialized) {
    Serial1.end();

    Serial1.begin(115200, SERIAL_8N1);
    assertTrue(Serial1.isEnabled());

    Wire.end();
    Wire.begin();
    assertTrue(Wire.isEnabled());
    assertEqual(getPinAf(D0), (uint32_t)GPIO_AF_I2C1);

    Serial1.end();
    assertEqual(getPinAf(D0), (uint32_t)GPIO_AF_I2C1);
    Wire.end();
}

#endif // HAL_PLATFORM_GEN == 2

test(UART_02_PinConfigurationNotAffectedWhenEndCalledMultipleTimes) {
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
