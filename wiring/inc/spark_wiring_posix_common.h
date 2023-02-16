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

#ifndef SPARK_WIRING_POSIX_COMMON_H
#define SPARK_WIRING_POSIX_COMMON_H

#include "hal_platform.h"

#if HAL_USE_SOCKET_HAL_POSIX

#include "spark_wiring_platform.h"
#include <arpa/inet.h>

namespace spark {

namespace detail {

inline void sockaddrToIpAddressPort(const struct sockaddr* saddr, IPAddress& addr, uint16_t* port) {
    if (saddr->sa_family == AF_INET) {
        const struct sockaddr_in* inaddr = (const struct sockaddr_in*)saddr;
        addr = (const uint8_t*)(&inaddr->sin_addr.s_addr);
        if (port) {
            *port = ntohs(inaddr->sin_port);
        }
    }
#if HAL_IPv6
    else if (saddr->sa_family == AF_INET6) {
        const struct sockaddr_in6* in6addr = (const struct sockaddr_in6*)saddr;
        HAL_IPAddress a = {};
        if (!IN6_IS_ADDR_V4MAPPED(&in6addr->sin6_addr)) {
            memcpy(a.ipv6, in6addr->sin6_addr.s6_addr, sizeof(a.ipv6));
            a.v = 6;
            addr = IPAddress(a);
        } else {
            auto ptr = (const uint32_t*)(in6addr->sin6_addr.s6_addr);
            addr = (const uint8_t*)(&ptr[3]);
        }
        if (port) {
            *port = ntohs(in6addr->sin6_port);
        }
    }
#endif // HAL_IPv6
}

inline void ipAddressPortToSockaddr(const IPAddress& addr, uint16_t port, struct sockaddr* saddr) {
    if (addr.version() == 6) {
        struct sockaddr_in6* in6addr = (struct sockaddr_in6*)saddr;
        in6addr->sin6_len = sizeof(sockaddr_in6);
        in6addr->sin6_family = AF_INET6;
        in6addr->sin6_port = htons(port);
        const auto& a = addr.raw();
        memcpy(in6addr->sin6_addr.s6_addr, a.ipv6, sizeof(a.ipv6));
    }
#if HAL_IPv6
    else if (addr.version() == 4) {
        struct sockaddr_in* inaddr = (struct sockaddr_in*)saddr;
        inaddr->sin_len = sizeof(sockaddr_in);
        inaddr->sin_family = AF_INET;
        inaddr->sin_port = htons(port);
        const auto& a = addr.raw();
        // NOTE: HAL_IPAddress.ipv4 is host-order :|
        inaddr->sin_addr.s_addr = htonl(a.ipv4);
    }
#endif // HAL_IPv6
}

inline uint8_t netmaskToPrefixLength(struct sockaddr* saddr) {
    if (saddr->sa_family == AF_INET) {
        struct sockaddr_in* inaddr = (struct sockaddr_in*)saddr;
        uint32_t addr = ntohl(inaddr->sin_addr.s_addr);
        if (addr == 0) {
            return 0;
        }
        return sizeof(addr) * CHAR_BIT - __builtin_ctz(addr);
    } else if (saddr->sa_family == AF_INET6) {
        struct sockaddr_in6* in6addr = (struct sockaddr_in6*)saddr;
        uint8_t sum = 0;
        for (size_t i = 0; i < sizeof(in6addr->sin6_addr.s6_addr); i++) {
            if (in6addr->sin6_addr.s6_addr[i] == 0) {
                break;
            }
            sum += CHAR_BIT - __builtin_ctz(in6addr->sin6_addr.s6_addr[i]);
        }
        return sum;
    }

    return 0;
}

inline void prefixLengthToNetmask(struct sockaddr* saddr, uint8_t prefixLen) {
    if (saddr->sa_family == AF_INET) {
        struct sockaddr_in* inaddr = (struct sockaddr_in*)saddr;
        uint32_t addr = 0xffffffff;
        auto shift = (sizeof(addr) * CHAR_BIT - prefixLen);
        addr >>= shift;
        addr <<= shift;
        inaddr->sin_addr.s_addr = htonl(addr);
    } else if (saddr->sa_family == AF_INET6) {
        struct sockaddr_in6* in6addr = (struct sockaddr_in6*)saddr;
        for (size_t i = 0; i < sizeof(in6addr->sin6_addr.s6_addr); i++) {
            if (prefixLen == 0) {
                break;
            } else if (prefixLen >= CHAR_BIT) {
                prefixLen -= CHAR_BIT;
                in6addr->sin6_addr.s6_addr[i] = 0xff;
            } else {
                in6addr->sin6_addr.s6_addr[i] = 0xff;
                in6addr->sin6_addr.s6_addr[i] >>= CHAR_BIT - prefixLen;
                in6addr->sin6_addr.s6_addr[i] <<= CHAR_BIT - prefixLen;
            }
        }
    }
}

} // detail

} // common

#endif // HAL_USE_SOCKET_HAL_POSIX

#endif // SPARK_WIRING_POSIX_COMMON_H
