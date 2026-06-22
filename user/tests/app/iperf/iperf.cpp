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

#include <sys/socket.h>
#include <arpa/inet.h>

#include "ifapi.h"
#include "iperf/iperf.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

namespace {

Serial1LogHandler logHandler(460800, LOG_LEVEL_ALL, {
    { "app", LOG_LEVEL_ALL },
    { "iperf", LOG_LEVEL_ALL }
});

particle::IperfServer iperf;

void logInterfaceAddresses() {
    struct if_addrs* addrs = nullptr;
    if (if_get_if_addrs(&addrs) != 0) {
        LOG(WARN, "if_get_if_addrs() failed");
        return;
    }
    for (auto a = addrs; a != nullptr; a = a->next) {
        if (!a->if_addr || !a->if_addr->addr) {
            continue;
        }
        auto addr = a->if_addr->addr;
        char buf[48] = {};
        if (addr->sa_family == AF_INET) {
            inet_ntop(AF_INET, &((const struct sockaddr_in*)addr)->sin_addr, buf, sizeof(buf));
        } else if (addr->sa_family == AF_INET6) {
            inet_ntop(AF_INET6, &((const struct sockaddr_in6*)addr)->sin6_addr, buf, sizeof(buf));
        } else {
            continue;
        }
        LOG(INFO, "%s: %s", a->ifname ? a->ifname : "?", buf);
    }
    if_free_if_addrs(addrs);
}

template <typename NetworkT>
void checkNetworkReady(NetworkT& network, const char* name, bool& wasReady) {
    bool ready = network.ready();
    if (ready && !wasReady) {
        LOG(INFO, "%s is ready", name);
        logInterfaceAddresses();
    } else if (!ready && wasReady) {
        LOG(WARN, "%s is no longer ready", name);
    }
    wasReady = ready;
}

} // namespace

void setup() {
    waitUntil(Serial.isConnected);
    Particle.connect();
#if HAL_PLATFORM_PPP_SERVER
    Tether.bind(TetherUSBConfig());
    Tether.on();
    Tether.connect();
#endif // HAL_PLATFORM_PPP_SERVER
    iperf.quiet().priority(OS_THREAD_PRIORITY_DEFAULT + 1).start();
}

void loop() {
#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
    static bool wifiWasReady = false;
    checkNetworkReady(WiFi, "WiFi", wifiWasReady);
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
#if HAL_PLATFORM_CELLULAR
    static bool cellularWasReady = false;
    checkNetworkReady(Cellular, "Cellular", cellularWasReady);
#endif // HAL_PLATFORM_CELLULAR
#if HAL_PLATFORM_ETHERNET
    static bool ethernetWasReady = false;
    checkNetworkReady(Ethernet, "Ethernet", ethernetWasReady);
#endif // HAL_PLATFORM_ETHERNET
#if HAL_PLATFORM_PPP_SERVER
    static bool tetherWasReady = false;
    checkNetworkReady(Tether, "Tether", tetherWasReady);
#endif // HAL_PLATFORM_PPP_SERVER
    delay(100);
}
