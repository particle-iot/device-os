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

test(01_TETHERING_SERIAL1_setup_and_bind) {
    // M-HAT: enable channel 2 of the UART switch to route Serial1 (TX/RX/CTS/RTS)
    // through to the Particle Debugger USB-serial bridge.
    pinMode(A0, OUTPUT);
    digitalWrite(A0, HIGH);

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, 60000));
    Cellular.connect();
    assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    Tether.bind(TetherSerialConfig().baudrate(921600).serial(Serial1));
    Tether.on();
    Tether.connect();

    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}

test(02_TETHERING_SERIAL1_test_download_speeds) {
    // Host-side: JS fixture spawns the tethering docker container.
}
