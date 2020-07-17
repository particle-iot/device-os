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

test(NETWORK_00_UDP_begin_does_not_leak_sockets_without_calling_stop) {
    // Arbitrary number that is large enough to showcase the issue
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
    // There is a limited number of sockets available on Electrons, since we are using
    // the internal modem TCP/IP stack. Reducing the number of iterations, because each
    // UDP.begin() call will cause a call into the modem using AT commands.
    // 10 should be enough to showcase the issue.
    int maxIterations = 10;
#else
    int maxIterations = 1000;
#endif // PLATFORM_ID == PLATFORM_ELECTRON

    auto udp = std::make_unique<UDP>();
    assertTrue((bool)udp);

    while (--maxIterations >= 0) {
        assertTrue(Network.ready());
        assertTrue((bool)udp->begin(12345));
        assertTrue((bool)socket_handle_valid(udp->socket()));
    }
}

test(NETWORK_01_UDP_sockets_can_be_read_with_timeout) {
    auto udp = std::make_unique<UDP>();
    assertTrue((bool)udp);

    assertTrue(Network.ready());
    assertTrue((bool)udp->begin(12345));

    const system_tick_t timeout = 1000;

    auto t = millis();
    udp->parsePacket(timeout);
    assertMoreOrEqual(millis(), t + timeout - timeout / 10);

    t = millis();
    udp->parsePacket(0);
    assertLess(millis(), t + timeout / 2);
}
