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

#include "hal_platform.h"

#if HAL_USE_SOCKET_HAL_POSIX

#include "spark_wiring_udp.h"
#include "socket_hal.h"
#include "inet_hal.h"
#include "spark_macros.h"
#include "spark_wiring_network.h"
#include "check.h"
#include "scope_guard.h"
#if HAL_PLATFORM_IFAPI
#include "ifapi.h"
#endif // HAL_PLATFORM_IFAPI
#include "netdb_hal.h"
#include <arpa/inet.h>
#include "spark_wiring_constants.h"
#include "spark_wiring_posix_common.h"

using namespace spark;

namespace {

inline bool isOpen(sock_handle_t sd) {
    return socket_handle_valid(sd);
}

int joinLeaveMulticast(int sock, const IPAddress& addr, uint8_t ifindex, bool join) {
    sockaddr_storage s = {};
    detail::ipAddressPortToSockaddr(addr, 0, (struct sockaddr*)&s);
    if (s.ss_family == AF_INET) {
        struct ip_mreq mreq = {};
        mreq.imr_multiaddr = ((struct sockaddr_in*)&s)->sin_addr;
        mreq.imr_interface.s_addr = INADDR_ANY;
#if HAL_PLATFORM_IFAPI
        if (ifindex != 0) {
            if_t iface;
            if (!if_get_by_index(ifindex, &iface)) {
                struct if_addrs* addrs = nullptr;
                SCOPE_GUARD({
                    if_free_if_addrs(addrs);
                });
                // Query interface IP address
                if (!if_get_addrs(iface, &addrs)) {
                    for (auto a = addrs; a != nullptr; a = a->next) {
                        auto ifaddr = a->if_addr->addr;
                        if (ifaddr->sa_family == AF_INET) {
                            struct sockaddr_in* inaddr = (struct sockaddr_in*)ifaddr;
                            if (inaddr->sin_addr.s_addr != INADDR_ANY) {
                                mreq.imr_interface.s_addr = inaddr->sin_addr.s_addr;
                                break;
                            }
                        }
                    }
                }
            }
        }
#endif // HAL_PLATFORM_IFAPI
        return sock_setsockopt(sock, IPPROTO_IP, join ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP,
                    (void*)&mreq, sizeof(mreq));
#if HAL_IPv6
    } else if (s.ss_family == AF_INET6) {
        struct ipv6_mreq mreq = {};
        mreq.ipv6mr_multiaddr = ((struct sockaddr_in6*)&s)->sin6_addr;
        mreq.ipv6mr_interface = ifindex;
        CHECK(sock_setsockopt(sock, IPPROTO_IPV6, join ? IPV6_JOIN_GROUP : IPV6_LEAVE_GROUP,
                &mreq, sizeof(mreq)));
        return 0;
    }
#endif // HAL_IPv6
    return -1;
}

} // anonymous

UDP::UDP()
        : _sock(-1),
          _offset(0),
          _total(0),
          _buffer(0),
          _buffer_size(512),
          _nif(0) {
}

bool UDP::setBuffer(size_t buf_size, uint8_t* buffer) {
    releaseBuffer();

    _buffer = buffer;
    _buffer_size = 0;
    if (!_buffer && buf_size) {         // requested allocation
        _buffer = new uint8_t[buf_size];
        _buffer_allocated = true;
    }
    if (_buffer) {
        _buffer_size = buf_size;
    }
    return _buffer_size;
}

void UDP::releaseBuffer() {
    if (_buffer_allocated && _buffer) {
        delete _buffer;
    }
    _buffer = NULL;
    _buffer_allocated = false;
    _buffer_size = 0;
    flush_buffer(); // clear buffer
}

uint8_t UDP::begin(uint16_t port, network_interface_t nif) {
    stop();

    bool bound = false;
    const int one = 1;

#if HAL_IPv6
    struct sockaddr_in6 saddr = {};
    saddr.sin6_len = sizeof(saddr);
    saddr.sin6_family = AF_INET6;
    saddr.sin6_port = htons(port);
    // saddr.sin6_addr = in6addr_any;
#else
    struct sockaddr_in saddr = {};
    saddr.sin_len = sizeof(saddr);
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = INADDR_ANY;
#endif // HAL_IPv6

    // Create socket
    _sock = sock_socket(HAL_IPv6 ? AF_INET6 : AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (_sock < 0) {
        goto done;
    }

    if (sock_setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one))) {
        goto done;
    }

#if HAL_PLATFORM_IFAPI
    // TODO: provide compatibility headers and use if_indextoname()
    if (nif != 0) {
        struct ifreq ifr = {};
        if (if_index_to_name(nif, ifr.ifr_name)) {
            goto done;
        }
        if (sock_setsockopt(_sock, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr))) {
            goto done;
        }
    }
#endif // HAL_PLATFORM_IFAPI

    // Bind socket
    if (sock_bind(_sock, (const struct sockaddr*)&saddr, sizeof(saddr))) {
        goto done;
    }

    _nif = nif;

    bound = true;

done:
    if (!bound) {
        stop();
    }
    return bound;
}

int UDP::available() {
    return _total - _offset;
}

void UDP::stop() {
    if (isOpen(_sock)) {
        sock_close(_sock);
    }

    _sock = -1;

    flush_buffer(); // clear buffer
}

int UDP::beginPacket(const char *host, uint16_t port) {
    struct addrinfo* ais = nullptr;
    SCOPE_GUARD({
        netdb_freeaddrinfo(ais);
    });
    CHECK(netdb_getaddrinfo(host, nullptr, nullptr, &ais));

    // FIXME: for now using only the first entry
    if (ais && ais->ai_addr) {
        IPAddress addr;
        detail::sockaddrToIpAddressPort(ais->ai_addr, addr, nullptr);
        if (addr) {
            return beginPacket(addr, port);
        }
    }

    return -1;
}

int UDP::beginPacket(IPAddress ip, uint16_t port) {
	LOG_DEBUG(TRACE, "begin packet %s#%d", ip.toString().c_str(), port);
    // default behavior previously was to use a 512 byte buffer, so instantiate that if not already done
    if (!_buffer && _buffer_size) {
        setBuffer(_buffer_size);
    }

    _remoteIP = ip;
    _remotePort = port;
    flush_buffer(); // clear buffer
    return _buffer_size;
}

int UDP::endPacket() {
    int result = sendPacket(_buffer, _offset, _remoteIP, _remotePort);
    flush(); // wait for send to complete
    return result;
}

int UDP::sendPacket(const uint8_t* buffer, size_t buffer_size, IPAddress remoteIP, uint16_t port) {
    LOG_DEBUG(TRACE, "sendPacket size %d, %s#%d", buffer_size, remoteIP.toString().c_str(), port);
	sockaddr_storage s = {};
    detail::ipAddressPortToSockaddr(remoteIP, port, (struct sockaddr*)&s);
    if (s.ss_family == AF_UNSPEC) {
        return -1;
    }

    return sock_sendto(_sock, buffer, buffer_size, 0, (const struct sockaddr*)&s, sizeof(s));
}

size_t UDP::write(uint8_t byte) {
    return write(&byte, 1);
}

size_t UDP::write(const uint8_t *buffer, size_t size) {
    size_t available = _buffer ? _buffer_size - _offset : 0;
    if (size > available) {
        size = available;
    }
    memcpy(_buffer + _offset, buffer, size);
    _offset += size;
    return size;
}

int UDP::parsePacket(system_tick_t timeout) {
    if (!_buffer && _buffer_size) {
        setBuffer(_buffer_size);
    }

    flush_buffer();         // start a new read - discard the old data
    if (_buffer && _buffer_size) {
        int result = receivePacket(_buffer, _buffer_size, timeout);
        if (result > 0) {
            _total = result;
        }
    }
    return available();
}

int UDP::receivePacket(uint8_t* buffer, size_t size, system_tick_t timeout) {
    int ret = -1;
    if (isOpen(_sock) && buffer) {
        sockaddr_storage saddr = {};
        socklen_t slen = sizeof(saddr);
        int flags = 0;
        if (timeout == 0) {
            flags = MSG_DONTWAIT;
        } else {
            struct timeval tv = {};
            tv.tv_sec = timeout / 1000;
            tv.tv_usec = (timeout % 1000) * 1000;
            ret = sock_setsockopt(_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            if (ret) {
                return ret;
            }
        }
        ret = sock_recvfrom(_sock, buffer, size, flags, (struct sockaddr*)&saddr, &slen);
        if (ret >= 0) {
            detail::sockaddrToIpAddressPort((const struct sockaddr*)&saddr, _remoteIP, &_remotePort);
            LOG_DEBUG(TRACE, "received %d bytes from %s#%d", ret, _remoteIP.toString().c_str(), _remotePort);
        }
    }
    return ret;
}

int UDP::read() {
    return available() ? _buffer[_offset++] : -1;
}

int UDP::read(unsigned char* buffer, size_t len) {
    int read = -1;
    if (available()) {
        read = min(int(len), available());
        memcpy(buffer, &_buffer[_offset], read);
        _offset += read;
    }
    return read;
}

int UDP::peek() {
    return available() ? _buffer[_offset] : -1;
}

void UDP::flush() {
}

void UDP::flush_buffer() {
    _offset = 0;
    _total = 0;
}

size_t UDP::printTo(Print& p) const {
    // can't use available() since this is a `const` method, and available is part of the Stream interface, and is non-const.
    int size = _total - _offset;
    return p.write(_buffer + _offset, size);
}

int UDP::joinMulticast(const IPAddress& ip) {
    if (!isOpen(_sock)) {
        return -1;
    }
    return joinLeaveMulticast(_sock, ip, _nif, true);
}

int UDP::leaveMulticast(const IPAddress& ip) {
    if (!isOpen(_sock)) {
        return -1;
    }
    return joinLeaveMulticast(_sock, ip, _nif, false);
}

#endif // HAL_USE_SOCKET_HAL_COMPAT
