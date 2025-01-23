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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "socket_hal_posix.h"
#include "netdb_hal.h"
#include "inet_hal_posix.h"

// These are internal LwIP headers, which is not ideal
#include <lwip/prot/icmp.h>
#include <lwip/prot/icmp6.h>
#include <lwip/prot/ip6.h>
#include <lwip/prot/ip4.h>

#include "check.h"
#include "scope_guard.h"
#include "random.h"
#include "system_error.h"
#include "system_network.h"
#include <memory>
#include "timer_hal.h"

namespace particle {
namespace services {
namespace detail {

/*
 * RFC 1071 - http://tools.ietf.org/html/rfc1071
 */
inline uint16_t ipChecksum(const uint8_t *buf, size_t size) {
    size_t i;
    uint64_t sum = 0;

    for (i = 0; i < size; i += 2) {
        sum += *(uint16_t*)buf;
        buf += 2;
    }
    if (size - i > 0) {
        sum += *(uint8_t*)buf;
    }

    while ((sum >> 16) != 0) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return (uint16_t)~sum;
}

inline void fillPingData(uint8_t* buffer, size_t bufferSize) {
    for (size_t i = 0; i < bufferSize; i++) {
        buffer[i] = '0' + i;
    }
}

inline bool validatePingData(const uint8_t* buffer, size_t bufferSize) {
    for (size_t i = 0; i < bufferSize; i++) {
        if (buffer[i] != ('0' + i)) {
            return false;
        }
    }

    return true;
}

inline int createIcmpPacket(int af, uint16_t id, uint16_t seq, uint8_t* buffer, size_t bufferSize) {
    if (af == AF_INET) {
        CHECK_TRUE(bufferSize >= sizeof(icmp_echo_hdr), SYSTEM_ERROR_INVALID_ARGUMENT);
        auto hdr = reinterpret_cast<icmp_echo_hdr*>(buffer);
        hdr->type = ICMP_ECHO;
        hdr->code = 0;
        hdr->chksum = 0;
        hdr->id = htons(id);
        hdr->seqno = htons(seq);

        fillPingData(buffer + sizeof(icmp_echo_hdr), bufferSize - sizeof(icmp_echo_hdr));
        hdr->chksum = ipChecksum(buffer, bufferSize);
        return 0;
    } else if (af == AF_INET6) {
        CHECK_TRUE(bufferSize >= sizeof(icmp6_echo_hdr), SYSTEM_ERROR_INVALID_ARGUMENT);
        auto hdr = reinterpret_cast<icmp6_echo_hdr*>(buffer);
        hdr->type = ICMP6_TYPE_EREQ;
        hdr->code = 0;
        hdr->chksum = 0;
        hdr->id = htons(id);
        hdr->seqno = htons(seq);

        fillPingData(buffer + sizeof(icmp6_echo_hdr), bufferSize - sizeof(icmp6_echo_hdr));
        // We don't need to calculate checksum here, as it will done for us by the TCP/IP stack
        return 0;
    }
    return SYSTEM_ERROR_INVALID_ARGUMENT;
}

inline bool validateIcmpPacket(int af, uint16_t id, uint16_t seq, const uint8_t* buffer, int bufferSize, size_t expected,
        const char* host, const struct sockaddr* from, system_tick_t elapsed) {

    char fromAddress[INET6_ADDRSTRLEN] = {};
    netdb_getnameinfo(from, from->sa_len, fromAddress, sizeof(fromAddress), nullptr, 0, NI_NUMERICHOST | NI_NUMERICSERV);
    // NOTE: we rely on the TCP/IP stack to validate checksums for us
    if (af == AF_INET) {
        if ((bufferSize - sizeof(ip_hdr)) != expected) {
            return false;
        }
        buffer += sizeof(ip_hdr);
        bufferSize -= sizeof(ip_hdr);
        auto hdr = reinterpret_cast<const icmp_echo_hdr*>(buffer);
        bool ok = hdr->type == ICMP_ER && hdr->code == 0 && ntohs(hdr->id) == id && ntohs(hdr->seqno) <= seq &&
                validatePingData(buffer + sizeof(*hdr), bufferSize - sizeof(*hdr));
        if (ok) {
            uint16_t seqno = ntohs(hdr->seqno);
            (void)seqno;
            // LOG(TRACE, "Received %d bytes from %s (%s), seq = %d, time = %lums", bufferSize, host, fromAddress, seqno, seqno == seq ? elapsed : 0);
        }
        return ok;
    } else {
        if (bufferSize - sizeof(ip6_hdr) < expected) {
            return false;
        }
        // Skip IPv6 header with any extensions
        // FIXME: it's better to parse everything
        buffer = (buffer + bufferSize) - expected;
        bufferSize = expected;
        auto hdr = reinterpret_cast<const icmp6_echo_hdr*>(buffer);
        bool ok = hdr->type == ICMP6_TYPE_EREP && hdr->code == 0 && ntohs(hdr->id) == id && ntohs(hdr->seqno) <= seq &&
                validatePingData(buffer + sizeof(*hdr), bufferSize - sizeof(*hdr));
        if (ok) {
            uint16_t seqno = ntohs(hdr->seqno);
            (void)seqno;
            // LOG(TRACE, "Received %d bytes from %s (%s), seq = %d, time = %lums", bufferSize, host, fromAddress, seqno, seqno == seq ? elapsed : 0);
        }
        return ok;
    }
}

} // detail

inline int ping(const char* host, unsigned count = 1, system_tick_t timeoutMs = 1000, network_interface_t pingIface = NETWORK_INTERFACE_ALL, int addressFamily = AF_UNSPEC) {
    static const constexpr size_t packetSize = 64;

    struct addrinfo* ais = nullptr;
    struct addrinfo hints = {};
    hints.ai_family = addressFamily;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_flags = AI_ADDRCONFIG;

    CHECK_TRUE(netdb_getaddrinfo(host, nullptr, &hints, &ais) == 0, SYSTEM_ERROR_NOT_FOUND);
    SCOPE_GUARD({
        netdb_freeaddrinfo(ais);
    });

    int fd = -1;
    const struct addrinfo* resolved = nullptr;
    for (auto ai = ais; ai != nullptr; ai = ai->ai_next) {
        // FIXME: This is not ideal and we should find a better solution.
        if (!network_ready(pingIface, ai->ai_family == AF_INET ? NETWORK_READY_TYPE_IPV4 : NETWORK_READY_TYPE_IPV6, nullptr)) {
            continue;
        }
        fd = sock_socket(ai->ai_family, ai->ai_socktype, ai->ai_family == AF_INET ? IPPROTO_ICMP : IPPROTO_ICMPV6);
        if (fd >= 0) {
            resolved = ai;
            char resolvedAddress[INET6_ADDRSTRLEN] = {};
            netdb_getnameinfo(ai->ai_addr, ai->ai_addrlen, resolvedAddress, sizeof(resolvedAddress), nullptr, 0, NI_NUMERICHOST | NI_NUMERICSERV);
            // LOG(TRACE, "Resolved %s to %s", host, resolvedAddress);
            break;
        }
    }
    CHECK_TRUE(fd >= 0, SYSTEM_ERROR_NETWORK);

    if (pingIface != NETWORK_INTERFACE_ALL) {
        struct ifreq ifr = {};
        CHECK_TRUE(if_index_to_name(pingIface, ifr.ifr_name) == 0, SYSTEM_ERROR_INTERNAL);
        CHECK_TRUE(sock_setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) == 0, SYSTEM_ERROR_INTERNAL);
    }

    SCOPE_GUARD({
        sock_close(fd);
    });

    const size_t bufferSize = packetSize + (resolved->ai_family == AF_INET ? sizeof(icmp_echo_hdr) : sizeof(icmp6_echo_hdr));
    auto buffer = std::make_unique<uint8_t[]>(bufferSize * 2);
    CHECK_TRUE(buffer, SYSTEM_ERROR_NO_MEMORY);

    Random r;
    uint16_t pingId = r.gen<uint16_t>();

    int replies = 0;
    for (unsigned i = 0; i < count; i++) {
        uint16_t seq = i + 1;
        CHECK(detail::createIcmpPacket(resolved->ai_family, pingId, seq, buffer.get(), bufferSize));
        // Send echo request
        CHECK_TRUE(sock_sendto(fd, buffer.get(), bufferSize, 0, resolved->ai_addr, resolved->ai_addrlen) == (int)bufferSize, SYSTEM_ERROR_INTERNAL);
        // Wait for echo reply up to timeoutMs milliseconds
        auto start = HAL_Timer_Get_Milli_Seconds();
        do {
            struct timeval tv = {};
            auto timeout = std::max((system_tick_t)1, timeoutMs - (HAL_Timer_Get_Milli_Seconds() - start));
            tv.tv_sec = timeout / 1000;
            tv.tv_usec = (timeout - (tv.tv_sec * 1000)) * 1000;
            CHECK_TRUE(sock_setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0, SYSTEM_ERROR_INTERNAL);
            struct sockaddr_storage sa = {};
            socklen_t saLen = sizeof(sa);
            int r = sock_recvfrom(fd, buffer.get(), bufferSize * 2, 0, (struct sockaddr*)&sa, &saLen);
            if (r < 0) {
                if (errno == EAGAIN) {
                    // Timeout
                    // LOG(TRACE, "Timeout");
                    continue;
                } else {
                    return SYSTEM_ERROR_NETWORK;
                }
            }
            auto received = HAL_Timer_Get_Milli_Seconds();

            if (detail::validateIcmpPacket(resolved->ai_family, pingId, seq, buffer.get(), r, bufferSize, host, (const struct sockaddr*)&sa, received - start)) {
                replies++;
                break;
            }
        } while (HAL_Timer_Get_Milli_Seconds() - start < timeoutMs);
    }

    return replies;
}

} // particle::services
} // particle
