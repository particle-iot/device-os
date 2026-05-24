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

test(01_TETHERING_USB_setup_and_bind) {
    // M-HAT only: enable channel 2 of the UART switch (TX/RX/CTS/RTS). No-op
    // on platforms without the switch; kept identical to the Serial1 variant
    // so the two binaries diverge only at Tether.bind().
    pinMode(A0, OUTPUT);
    digitalWrite(A0, HIGH);

    Cellular.on();
    assertTrue(waitFor(Cellular.isOn, 60000));
    Cellular.connect();
    assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    Tether.bind(TetherUSBConfig());
    Tether.on();
    Tether.connect();

    // Particle.connect proves the cellular data path is up before we hand
    // off to the JS fixture. The fixture polls ppp0 separately on the host.
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}

test(02_TETHERING_USB_test_download_speeds) {
    // Host-side: JS fixture spawns the tethering docker container.
}
