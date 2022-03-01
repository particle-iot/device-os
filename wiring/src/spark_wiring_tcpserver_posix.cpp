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
#include "spark_wiring_tcpserver.h"
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
#include "spark_wiring_thread.h"
#include "spark_wiring_posix_common.h"

using namespace spark;

static TCPClient* s_invalid_client = nullptr;

class TCPServerClient : public TCPClient {
public:
    TCPServerClient(sock_handle_t sock) : TCPClient(sock) {
    }

    virtual IPAddress remoteIP() override {
        IPAddress addr;
        struct sockaddr_storage saddr = {};
        socklen_t len = sizeof(saddr);
        if (!sock_getpeername(sock_handle(), (struct sockaddr*)&saddr, &len)) {
            detail::sockaddrToIpAddressPort((const struct sockaddr*)&saddr, addr, nullptr);
        }
        return addr;
    }
};

TCPServer::TCPServer(uint16_t port, network_interface_t nif)
        : _port(port),
          _nif(nif),
          _sock(-1),
          _client(-1) {
    SINGLE_THREADED_BLOCK() {
        if (!s_invalid_client) {
            s_invalid_client = new TCPClient(-1);
        }
    }
}

bool TCPServer::begin() {
    stop();

    if (socket_handle_valid(_sock)) {
        return true;
    }

    NAMED_SCOPE_GUARD(done, {
        stop();
    });

#if HAL_IPv6
    struct sockaddr_in6 saddr = {};
    saddr.sin6_len = sizeof(saddr);
    saddr.sin6_family = AF_INET6;
    saddr.sin6_port = htons(_port);
    // saddr.sin6_addr = in6addr_any;
#else
    struct sockaddr_in saddr = {};
    saddr.sin_len = sizeof(saddr);
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(_port);
    saddr.sin_addr.s_addr = INADDR_ANY;
#endif // HAL_IPv6

    // Create socket
    _sock = sock_socket(HAL_IPv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP);
    CHECK_TRUE(_sock >= 0, false);

    const int one = 1;
    CHECK_TRUE(sock_setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == 0, false);

    int flags = sock_fcntl(_sock, F_GETFL, 0);
    CHECK_TRUE(flags >= 0, false);
    flags |= O_NONBLOCK;
    CHECK_TRUE(sock_fcntl(_sock, F_SETFL, flags) >= 0, false);

#if HAL_PLATFORM_IFAPI
    // TODO: provide compatibility headers and use if_indextoname()
    if (_nif != 0) {
        struct ifreq ifr = {};
        if (if_index_to_name(_nif, ifr.ifr_name)) {
            return false;
        }
        if (sock_setsockopt(_sock, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr))) {
            return false;
        }
    }
#endif // HAL_PLATFORM_IFAPI

    int r = sock_bind(_sock, (const struct sockaddr*)&saddr, sizeof(saddr));
    CHECK_TRUE(!r, false);

    r = sock_listen(_sock, 5);
    CHECK_TRUE(!r, false);

    done.dismiss();

    return true;
}

void TCPServer::stop() {
    _client.stop();
    sock_close(_sock);
    _sock = -1;
}

TCPClient TCPServer::available() {
    if (_sock < 0) {
        begin();
    }

    if (_sock < 0) {
        _client = *s_invalid_client;
        return _client;
    }

    struct sockaddr_storage saddr = {};
    socklen_t slen = sizeof(saddr);
    int s = sock_accept(_sock, (struct sockaddr*)&saddr, &slen);
    if (s < 0) {
        _client = *s_invalid_client;
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            stop();
        }
    } else {
        TCPServerClient client = TCPServerClient(s);
        client.d_->remoteIP = client.remoteIP(); // fetch the peer IP ready for the copy operator
        _client = client;
    }

    return _client;
}

size_t TCPServer::write(uint8_t b, system_tick_t timeout) {
    return write(&b, sizeof(b), timeout);
}

size_t TCPServer::write(const uint8_t *buf, size_t size, system_tick_t timeout) {
    _client.clearWriteError();
    size_t ret = _client.write(buf, size, timeout);
    setWriteError(_client.getWriteError());
    return ret;
}

size_t TCPServer::write(uint8_t b) {
    return write(&b, 1);
}

size_t TCPServer::write(const uint8_t *buffer, size_t size) {
    return write(buffer, size, SOCKET_WAIT_FOREVER);
}

#endif // HAL_USE_SOCKET_HAL_POSIX
