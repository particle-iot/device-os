/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

namespace {

// SPI1 pins: most Gen3 boards use D2/D3/D4 (nRF52840 spiMap) and don't define SCK1 etc.
#if defined(SCK1) && defined(MOSI1) && defined(MISO1)
const hal_pin_t spi1Pins[] = { SCK1, MOSI1, MISO1 };
#else
const hal_pin_t spi1Pins[] = { D2, D3, D4 };
#endif

// One PWM-capable pin per platform that isn't shared with Serial1/SPI/Wire
#if PLATFORM_ID == PLATFORM_P2 || PLATFORM_ID == PLATFORM_TRACKERM
const hal_pin_t pwmPin = A2;
#elif PLATFORM_ID == PLATFORM_ESOMX
const hal_pin_t pwmPin = A3;
#elif PLATFORM_ID == PLATFORM_TRACKER
const hal_pin_t pwmPin = D7;
#else
const hal_pin_t pwmPin = A0;
#endif

} // anonymous

test(PIN_AVAILABLE_01_invalid_pins_are_not_available) {
    assertFalse(pinAvailable(TOTAL_PINS));
    assertFalse(pinAvailable(PIN_INVALID));
}

test(PIN_AVAILABLE_02_serial1_tx_rx_unavailable_while_enabled) {
    Serial1.end();
    assertTrue(pinAvailable(TX));
    assertTrue(pinAvailable(RX));

    Serial1.begin(115200);
    assertTrue(Serial1.isEnabled());
    assertFalse(pinAvailable(TX));
    assertFalse(pinAvailable(RX));
#if defined(CTS) && defined(RTS)
    // Without flow control, CTS/RTS stay free for GPIO
    assertTrue(pinAvailable(CTS));
    assertTrue(pinAvailable(RTS));
#endif

    Serial1.end();
    assertTrue(pinAvailable(TX));
    assertTrue(pinAvailable(RX));
}

#if defined(CTS) && defined(RTS)
test(PIN_AVAILABLE_03_serial1_cts_rts_unavailable_with_flow_control) {
    Serial1.begin(115200, SERIAL_8N1 | SERIAL_FLOW_CONTROL_RTS_CTS);
    assertTrue(Serial1.isEnabled());
    assertFalse(pinAvailable(CTS));
    assertFalse(pinAvailable(RTS));

    Serial1.end();
    assertTrue(pinAvailable(CTS));
    assertTrue(pinAvailable(RTS));
}
#endif // defined(CTS) && defined(RTS)

test(PIN_AVAILABLE_04_spi_pins_unavailable_ss_stays_manual) {
    SPI.end();
    SPI.begin(SS);
    assertFalse(pinAvailable(SCK));
    assertFalse(pinAvailable(MOSI));
    assertFalse(pinAvailable(MISO));
    // SS is plain GPIO, so the app can drive chip select itself
    assertTrue(pinAvailable(SS));
    digitalWrite(SS, LOW);
    assertEqual((PinState)digitalRead(SS), LOW);
    digitalWrite(SS, HIGH);
    assertEqual((PinState)digitalRead(SS), HIGH);

    SPI.end();
    assertTrue(pinAvailable(SCK));
    assertTrue(pinAvailable(MOSI));
    assertTrue(pinAvailable(MISO));
}

test(PIN_AVAILABLE_05_spi1_pins_unavailable_while_enabled) {
    SPI1.end();
    SPI1.begin();
    for (auto p : spi1Pins) {
        assertFalse(pinAvailable(p));
    }

    SPI1.end();
    for (auto p : spi1Pins) {
        assertTrue(pinAvailable(p));
    }
}

test(PIN_AVAILABLE_06_wire_pins_stay_unavailable_after_bus_reset) {
    Wire.begin();
    assertFalse(pinAvailable(SDA));
    assertFalse(pinAvailable(SCL));

    // Bus recovery goes through end() -> bit-bang -> begin() internally
    Wire.reset();
    assertFalse(pinAvailable(SDA));
    assertFalse(pinAvailable(SCL));

    Wire.end();
    assertTrue(pinAvailable(SDA));
    assertTrue(pinAvailable(SCL));
}

test(PIN_AVAILABLE_07_pinmode_cannot_hijack_peripheral_pin) {
    Serial1.begin(115200);
    pinMode(TX, INPUT_PULLDOWN);
    assertNotEqual(getPinMode(TX), INPUT_PULLDOWN);
    Serial1.end();

    SPI.begin();
    pinMode(SCK, INPUT_PULLDOWN);
    assertNotEqual(getPinMode(SCK), INPUT_PULLDOWN);
    SPI.end();

    Wire.begin();
    pinMode(SDA, INPUT_PULLDOWN);
    assertNotEqual(getPinMode(SDA), INPUT_PULLDOWN);
    Wire.end();
}

test(PIN_AVAILABLE_08_pwm_pin_remains_usable) {
    pinMode(pwmPin, OUTPUT);
    analogWrite(pwmPin, 128);
    assertTrue(pinAvailable(pwmPin));

    // Changing duty cycle and switching back to GPIO must still work
    analogWrite(pwmPin, 0);
    pinMode(pwmPin, INPUT_PULLDOWN);
    assertEqual(getPinMode(pwmPin), INPUT_PULLDOWN);
    pinMode(pwmPin, INPUT);
}
