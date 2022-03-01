/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "application.h"

SYSTEM_MODE(MANUAL);

extern const pin_t g_pins[];
extern const char* g_pin_names[];
extern const size_t g_pin_count;

void setup() {
}

void countdown(int c) {
    Serial.printf("Going into STOP mode in ");
    while (c > 0) {
        Serial.printf("%d", c--);
        delay(333);
        Serial.print(".");
        delay(333);
        Serial.print(".");
        delay(333);
    }
}

const char* getPinName(pin_t pin) {
    for (unsigned i = 0; i < g_pin_count; i++) {
        if (g_pins[i] == pin) {
            return g_pin_names[i];
        }
    }
    return "UNKNOWN";
}

void loop() {
    InterruptMode mode[g_pin_count];
    for (unsigned i = 0; i < sizeof(mode)/sizeof(*mode); i++) {
        mode[i] = (g_pins[i] == BTN ? FALLING : (i % 2 == 0 ? RISING : FALLING));
    }

    waitUntil(Serial.isConnected);
    SleepResult r = System.sleepResult();
    if (r.wokenUpByPin()) {
        Serial.printlnf("The device was woken up by pin %s %lu", getPinName(r.pin()), r.pin());
    } else if (r.wokenUpByRtc()) {
        Serial.printlnf("The device was woken up by RTC");
    }
    Serial.println("Press any key to enter STOP mode");
    Serial.println("You should be able to wake up your device using any of these pins:");
    for (unsigned i = 0; i < g_pin_count; i++) {
        Serial.printlnf("%s: %s", g_pin_names[i], mode[i] == RISING ? "RISING" : "FALLING");
    }
    Serial.println();
    Serial.println("The device will also automatically wake up after 60 seconds");
    while(Serial.available() <= 0) {
        Particle.process();
    }
    while(Serial.available() > 0) {
        Serial.read();
    }
    countdown(3);
    System.sleep(g_pins, g_pin_count, mode, g_pin_count, 60);
}
