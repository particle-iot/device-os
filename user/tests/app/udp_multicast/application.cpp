/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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
#include "ifapi.h"
#include "random.h"

static UDP udp;
static Random rng;
static IPAddress mcastAddr;
static const uint16_t PORT = 10000;
static const system_tick_t PERIOD = 1000;
static const char MULTICAST_ADDR[] = "ff03::1:1001";
static const size_t IP_ADDR_STRLEN_MAX = IP6ADDR_STRLEN_MAX;

Serial1LogHandler dbg(115200, LOG_LEVEL_ALL);

SYSTEM_MODE(SEMI_AUTOMATIC);

static uint8_t buf[1000] = {};

void openSocket() {
    udp.stop();

    // Get OpenThread interface index (OpenThread interface is named "th1" on all Mesh devices)
    uint8_t idx = 0;
    if_name_to_index("th1", &idx);

    Log.trace("th1 index = %u", idx);

    // Create UDP socket and bind to OpenThread interface
    auto ur = udp.begin(PORT, idx);
    Log.trace("UDP.begin() = %u", ur);

    // Subscribe to ff03::1:1001
    HAL_IPAddress addr = {};
    addr.v = 6;
    inet_inet_pton(AF_INET6, MULTICAST_ADDR, addr.ipv6);
    mcastAddr = addr;
    int r = udp.joinMulticast(mcastAddr);
    Log.trace("Subscribed to %s: %d", MULTICAST_ADDR, r);
}

/* executes once at startup */
void setup() {
    Mesh.connect();
    openSocket();
}

/* executes continuously after setup() runs */
void loop() {
    static system_tick_t last = 0;
    ssize_t s = udp.receivePacket(buf, sizeof(buf));
    if (s > 0) {
        char tmp[IP_ADDR_STRLEN_MAX] = {};
        auto ip = udp.remoteIP();
        Log.trace("Received %d bytes from %s#%u", s,
                inet_inet_ntop(AF_INET6, ip.raw().ipv6, tmp, sizeof(tmp)), udp.remotePort());
    }

    if ((millis() - last) >= PERIOD) {
        size_t len = random(1, sizeof(buf));
        rng.gen((char*)buf, len);
        auto r = udp.sendPacket(buf, len, mcastAddr, PORT);
        Log.trace("Sent %u bytes to %s#%u : %d %d", len, MULTICAST_ADDR, PORT, r, errno);
        last = millis();
        if (r < 0) {
            openSocket();
        }
    }
}
