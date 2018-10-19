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

#include "spark_wiring_tcpclient.h"
#include "socket_hal.h"
#include "inet_hal.h"
#include "spark_macros.h"
#include "spark_wiring_network.h"
#include "check.h"
#include "scope_guard.h"
#if HAL_PLATFORM_IFAPI
#include "ifapi.h"
#endif // HAL_PLATFORM_IFAPI
#include <netdb.h>
#include <arpa/inet.h>
#include "spark_wiring_constants.h"
#include "spark_wiring_posix_common.h"

using namespace spark;

static bool inline isOpen(sock_handle_t sd) {
    return socket_handle_valid(sd);
}

TCPClient::TCPClient()
        : TCPClient(-1) {
}

TCPClient::TCPClient(sock_handle_t sock)
        : d_(std::make_shared<Data>(sock)) {
    flush_buffer();
}

int TCPClient::connect(const char* host, uint16_t port, network_interface_t nif) {
    stop();

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
            return connect(addr, port, nif);
        }
    }

    return -1;
}

int TCPClient::connect(IPAddress ip, uint16_t port, network_interface_t nif) {
    stop();

    NAMED_SCOPE_GUARD(done, {
        stop();
    });

    d_->sock = sock_socket(ip.version() == 4 ? AF_INET : AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    CHECK(d_->sock);

    flush_buffer();

#if HAL_PLATFORM_IFAPI
    // TODO: provide compatibility headers and use if_indextoname()
    if (nif != 0) {
        struct ifreq ifr = {};
        CHECK(if_index_to_name(nif, ifr.ifr_name));
        CHECK(sock_setsockopt(d_->sock, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)));
    }
#endif // HAL_PLATFORM_IFAPI

    sockaddr_storage saddr = {};
    if (ip.version() == 4) {
        auto s = (sockaddr_in*)&saddr;
        s->sin_len = sizeof(saddr);
        s->sin_family = AF_INET;
        s->sin_port = htons(port);
        s->sin_addr.s_addr = htonl(ip.raw().ipv4);
    } else {
        auto s = (sockaddr_in6*)&saddr;
        s->sin6_len = sizeof(saddr);
        s->sin6_family = AF_INET6;
        s->sin6_port = htons(port);
        memcpy(s->sin6_addr.s6_addr, ip.raw().ipv6, sizeof(s->sin6_addr.s6_addr));
    }

    // FIXME: timeout?
    CHECK(sock_connect(d_->sock, (const sockaddr*)&saddr, sizeof(saddr)));

    d_->remoteIP = ip;

    done.dismiss();
    // Why not 0?
    return 1;
}

size_t TCPClient::write(uint8_t b) {
    return write(&b, 1, SOCKET_WAIT_FOREVER);
}

size_t TCPClient::write(const uint8_t *buffer, size_t size) {
    return write(buffer, size, SOCKET_WAIT_FOREVER);
}

size_t TCPClient::write(uint8_t b, system_tick_t timeout) {
    return write(&b, 1, timeout);
}

size_t TCPClient::write(const uint8_t *buffer, size_t size, system_tick_t timeout) {
    clearWriteError();
    struct timeval tv = {};
    if (timeout != SOCKET_WAIT_FOREVER) {
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
    }
    int ret = sock_setsockopt(d_->sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    if (ret < 0) {
        setWriteError(errno);
        return 0;
    }

    ret = sock_send(d_->sock, buffer, size, 0);
    if (ret < 0) {
        setWriteError(errno);
        return 0;
    }

    return ret;
}

int TCPClient::bufferCount() {
    return d_->total - d_->offset;
}

int TCPClient::available()
{
    int avail = 0;

    // At EOB => Flush it
    if (d_->total && (d_->offset == d_->total)) {
        flush_buffer();
    }

    if (isOpen(d_->sock)) {
        // Have room
        if (d_->total < arraySize(d_->buffer)) {
            int ret = sock_recv(d_->sock, d_->buffer + d_->total, arraySize(d_->buffer) - d_->total, MSG_DONTWAIT);
            if (ret > 0) {
                if (d_->total == 0) {
                    d_->offset = 0;
                }
                d_->total += ret;
            } else {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    LOG(ERROR, "recv error = %d", errno);
                    sock_close(d_->sock);
                    d_->sock = -1;
                }
            }
        } // Have Space
    } // isOpen(d_->sock)
    avail = bufferCount();
    return avail;
}

int TCPClient::read() {
    return (bufferCount() || available()) ? d_->buffer[d_->offset++] : -1;
}

int TCPClient::read(uint8_t *buffer, size_t size) {
    int read = -1;
    if (bufferCount() || available()) {
        read = (size > (size_t) bufferCount()) ? bufferCount() : size;
        memcpy(buffer, &d_->buffer[d_->offset], read);
        d_->offset += read;
    }
    return read;
}

int TCPClient::peek() {
    return (bufferCount() || available()) ? d_->buffer[d_->offset] : -1;
}

void TCPClient::flush_buffer() {
    d_->offset = 0;
    d_->total = 0;
}

void TCPClient::flush() {
}

void TCPClient::stop() {
    if (isOpen(d_->sock)) {
        sock_close(d_->sock);
    }
    d_->sock = -1;
    d_->remoteIP.clear();
    flush_buffer();
}

uint8_t TCPClient::connected() {
    bool rv = (status() || bufferCount());
    if (!rv) {
        rv = available();
        if (!rv) {
            stop();
        }
    }
    return rv;
}

uint8_t TCPClient::status() {
    return (isOpen(d_->sock));
}

TCPClient::operator bool() {
   return (status() != 0);
}

IPAddress TCPClient::remoteIP() {
    return d_->remoteIP;
}

TCPClient::Data::Data(sock_handle_t sock)
        : sock(sock),
          offset(0),
          total(0) {
}

TCPClient::Data::~Data() {
    if (socket_handle_valid(sock)) {
        sock_close(sock);
    }
}

#endif // HAL_USE_SOCKET_HAL_POSIX
