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

#include "spark_wiring_network.h"

#include "spark_wiring_wifi.h"
#include "spark_wiring_cellular.h"
#include "spark_wiring_mesh.h"
#include "spark_wiring_ethernet.h"
#include "hal_platform.h"
#if HAL_USE_INET_HAL_POSIX
#include <netdb.h>
#endif // HAL_USE_INET_HAL_POSIX

namespace spark {

NetworkClass Network(NETWORK_INTERFACE_ALL);

NetworkClass& NetworkClass::from(network_interface_t nif) {
    switch (nif) {
#if Wiring_Mesh
    case NETWORK_INTERFACE_MESH:
        return Mesh;
#endif
#if Wiring_Ethernet
    case NETWORK_INTERFACE_ETHERNET:
        return Ethernet;
#endif
#if Wiring_WiFi
    case NETWORK_INTERFACE_WIFI_STA:
        return WiFi;
    case NETWORK_INTERFACE_WIFI_AP:
        return Network; // FIXME
#endif
#if Wiring_Cellular
    case NETWORK_INTERFACE_CELLULAR:
        return Cellular;
#endif
    default:
        return Network;
    }
}

void NetworkClass::connect(unsigned flags) {
    network_connect(*this, flags, 0, nullptr);
}

void NetworkClass::disconnect() {
    network_disconnect(*this, NETWORK_DISCONNECT_REASON_USER, nullptr);
}

bool NetworkClass::connecting() {
    return network_connecting(*this, 0, nullptr);
}

bool NetworkClass::ready() {
    return network_ready(*this, 0, nullptr);
}

void NetworkClass::on() {
    network_on(*this, 0, 0, nullptr);
}

void NetworkClass::off() {
    network_off(*this, 0, 0, nullptr);
}

void NetworkClass::listen(bool begin) {
    network_listen(*this, begin ? 0 : 1, nullptr);
}

void NetworkClass::setListenTimeout(uint16_t timeout) {
    network_set_listen_timeout(*this, timeout, nullptr);
}

uint16_t NetworkClass::getListenTimeout() {
    return network_get_listen_timeout(*this, 0, nullptr);
}

bool NetworkClass::listening() {
    return network_listening(*this, 0, nullptr);
}

IPAddress NetworkClass::resolve(const char* name) {
    IPAddress addr;
#if HAL_USE_INET_HAL_POSIX
    struct addrinfo *ai = nullptr;
    struct addrinfo hints = {};
    hints.ai_flags = AI_ADDRCONFIG;
    hints.ai_family = AF_UNSPEC;
    const int r = getaddrinfo(name, nullptr, &hints, &ai);
    if (!r) {
        bool ok = false;
        // This is not really needed if AI_ADDRCONFIG is properly supported
        bool ipv4 = network_ready(*this, NETWORK_READY_TYPE_IPV4, nullptr);
        bool ipv6 = network_ready(*this, NETWORK_READY_TYPE_IPV6, nullptr);
        for (auto cur = ai; cur != nullptr && !ok; cur = cur->ai_next) {
            // NOTE: using only the first entry that matches the current state of IPv4/IPv6 connectivity
            switch (cur->ai_family) {
                case AF_INET: {
                    if (!ipv4) {
                        continue;
                    }
                    // NOTE: HAL_IPAddress is little-endian
                    auto in = (struct sockaddr_in*)cur->ai_addr;
                    addr = (const uint8_t*)(&in->sin_addr.s_addr);
                    ok = true;
                    break;
                }
                case AF_INET6: {
                    if (!ipv6) {
                        continue;
                    }
                    auto in6 = (struct sockaddr_in6*)cur->ai_addr;
                    HAL_IPAddress a = {};
                    a.v = 6;
                    memcpy(a.ipv6, in6->sin6_addr.s6_addr, sizeof(a.ipv6));
                    addr = IPAddress(a);
                    ok = true;
                    break;
                }
            }
        }
    }
    freeaddrinfo(ai);
#else

    // Compatibility calls into interface-specific methods for platforms
    // that do not support getaddrinfo()
#if Wiring_WiFi
    return WiFi.resolve(name);
#elif Wiring_Cellular
    return Cellular.resolve(name);
#endif // Wiring_Cellular

#endif // HAL_USE_INET_HAL_POSIX
    return addr;
}

} // spark
