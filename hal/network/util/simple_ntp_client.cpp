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

#include "logging.h"

LOG_SOURCE_CATEGORY("service.ntp");
#include "hal_platform.h"

#include "simple_ntp_client.h"
#include "check.h"

#include "inet_hal.h"

#if HAL_USE_SOCKET_HAL_POSIX
#include "netdb_hal.h"
#include <arpa/inet.h>
#else
#include "inet_hal_compat.h"
#include "delay_hal.h"
#include "system_defs.h"
#endif // HAL_USE_SOCKET_HAL_POSIX

#include "random.h"
#include "system_tick_hal.h"
#include "rtc_hal.h"
#include <ctime>
#include "timer_hal.h"
#include <cstdio>
#include "simple_ntp_client_detail.h"

namespace particle {

class UdpSocket {
public:
    UdpSocket()
            : UdpSocket(-1) {
    }

    UdpSocket(sock_handle_t sock)
            : sock_{sock},
#if HAL_USE_SOCKET_HAL_POSIX
              isOwnSock_{sock >= 0 ? false : true},
              addr_{nullptr},
              cached_{nullptr} {
#else
              isOwnSock_{socket_handle_valid(sock) ? false : true},
              addr_{} {
#endif // HAL_USE_SOCKET_HAL_POSIX
    }

    ~UdpSocket();

    int connect(const char* hostname, uint16_t port);
    ssize_t recv(uint8_t* buf, size_t size, system_tick_t timeout = 0);
    ssize_t send(const uint8_t* buf, size_t size);
    void close();

private:
    sock_handle_t sock_;
    bool isOwnSock_;

#if HAL_USE_SOCKET_HAL_POSIX
    struct addrinfo* addr_;
    struct addrinfo* cached_;
#else
    sockaddr_t addr_;
#endif // HAL_USE_SOCKET_HAL_POSIX
};

#if HAL_USE_SOCKET_HAL_POSIX

int UdpSocket::connect(const char* hostname, uint16_t port) {
    close();

    // FIXME: if new hostname is supplied which is different from what has been cached
    if (!cached_) {
        if (addr_) {
            netdb_freeaddrinfo(addr_);
            addr_ = nullptr;
            cached_ = nullptr;
        }
        // Perform DNS lookup
        struct addrinfo hints = {};
        hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;
        hints.ai_protocol = IPPROTO_UDP;
        hints.ai_socktype = SOCK_DGRAM;
        // FIXME: AF_INET only for now
        hints.ai_family = AF_INET;
        char tmpserv[8] = {};
        snprintf(tmpserv, sizeof(tmpserv), "%u", port);
        CHECK_TRUE(netdb_getaddrinfo(hostname, tmpserv, &hints, &addr_) == 0 && addr_, SYSTEM_ERROR_NOT_FOUND);
        cached_ = addr_;
    }

    int sock = -1;
    for (; cached_ != nullptr && sock < 0; cached_ = cached_->ai_next) {
        /* Iterate over all the addresses and attempt to connect */

        int s = -1;
        if (sock_ < 0) {
            s = sock_socket(cached_->ai_family, cached_->ai_socktype, cached_->ai_protocol);
            if (s < 0) {
                continue;
            }
        } else {
            s = sock_;
        }

#ifdef DEBUG_BUILD
        LOG_DEBUG(TRACE, "SNTP socket=%d, family=%d, type=%d, protocol=%d", s, cached_->ai_family, cached_->ai_socktype, cached_->ai_protocol);

        char serverHost[INET6_ADDRSTRLEN] = {};
        uint16_t serverPort = 0;
        switch (cached_->ai_family) {
            case AF_INET: {
                inet_inet_ntop(cached_->ai_family, &((sockaddr_in*)cached_->ai_addr)->sin_addr, serverHost, sizeof(serverHost));
                serverPort = bigEndianToNative(((sockaddr_in*)cached_->ai_addr)->sin_port);
                break;
            }
            case AF_INET6: {
                inet_inet_ntop(cached_->ai_family, &((sockaddr_in6*)cached_->ai_addr)->sin6_addr, serverHost, sizeof(serverHost));
                serverPort = bigEndianToNative(((sockaddr_in6*)cached_->ai_addr)->sin6_port);
                break;
            }
        }
        LOG_DEBUG(TRACE, "SNTP socket=%d, connecting to %s#%u", s, serverHost, serverPort);
#endif // DEBUG_BUILD

        int r = sock_connect(s, cached_->ai_addr, cached_->ai_addrlen);
        if (r) {
            if (s != sock_ || isOwnSock_) {
                sock_close(s);
            }
            continue;
        }

        sock = s;
        LOG_DEBUG(TRACE, "SNTP connected");
    }

    CHECK_TRUE(sock >= 0, SYSTEM_ERROR_NETWORK);
    sock_ = sock;
    return 0;
}

ssize_t UdpSocket::recv(uint8_t* buf, size_t size, system_tick_t timeout) {
    int flags = 0;
    if (timeout == 0) {
        flags = MSG_DONTWAIT;
    } else {
        struct timeval tv = {};
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        CHECK_TRUE(sock_setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0, SYSTEM_ERROR_INTERNAL);
    }
    ssize_t r = sock_recv(sock_, buf, size, flags);
    if (r >= 0) {
        return r;
    }

    if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return SYSTEM_ERROR_TIMEOUT;
    }

    return SYSTEM_ERROR_NETWORK;
}

ssize_t UdpSocket::send(const uint8_t* buf, size_t size) {
    CHECK_TRUE(sock_send(sock_, buf, size, MSG_DONTWAIT) == (ssize_t)size, SYSTEM_ERROR_NETWORK);
    return size;
}

void UdpSocket::close() {
    if (isOwnSock_ && sock_ >= 0) {
        sock_close(sock_);
        sock_ = -1;
    }
}

UdpSocket::~UdpSocket() {
    close();
    if (addr_) {
        netdb_freeaddrinfo(addr_);
        addr_ = nullptr;
    }
}

#else // !HAL_USE_SOCKET_HAL_POSIX

int UdpSocket::connect(const char* hostname, uint16_t port) {
    close();

    HAL_IPAddress ipAddr = {};
    int r = inet_gethostbyname(hostname, strlen(hostname), &ipAddr, NETWORK_INTERFACE_ALL, nullptr);
    if (r != 0) {
        return SYSTEM_ERROR_NOT_FOUND;
    }

    // XXX:
    addr_.sa_family = AF_INET;
    addr_.sa_data[0] = (port >> 8) & 0xff;
    addr_.sa_data[1] = (port & 0xff);
    addr_.sa_data[2] = (ipAddr.ipv4 >> 24) & 0xff;
    addr_.sa_data[3] = (ipAddr.ipv4 >> 16) & 0xff;
    addr_.sa_data[4] = (ipAddr.ipv4 >> 8) & 0xff;
    addr_.sa_data[5] = (ipAddr.ipv4) & 0xff;

    if (!socket_handle_valid(sock_)) {
        sock_ = socket_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, 0, NETWORK_INTERFACE_ALL);
        if (!socket_handle_valid(sock_)) {
            return SYSTEM_ERROR_INTERNAL;
        }
    }

    return 0;
}

ssize_t UdpSocket::recv(uint8_t* buf, size_t size, system_tick_t timeout) {
    auto start = HAL_Timer_Get_Milli_Seconds();
    auto end = start + timeout;
    system_tick_t now;
    while ((now = HAL_Timer_Get_Milli_Seconds()) <= end) {
        sockaddr_t remote = {};
        socklen_t remoteLen = sizeof(remote);
        ssize_t r = socket_receivefrom_ex(sock_, buf, size, 0, &remote, &remoteLen, end - now, nullptr);
        if (r >= 0) {
            if (remote.sa_family == addr_.sa_family &&
                    !memcmp(remote.sa_data, addr_.sa_data, sizeof(uint32_t) + sizeof(uint16_t))) {
                return r;
            }
        }
    }
    return SYSTEM_ERROR_TIMEOUT;
}

ssize_t UdpSocket::send(const uint8_t* buf, size_t size) {
    return socket_sendto(sock_, buf, size, 0, &addr_, sizeof(addr_));
}

void UdpSocket::close() {
    if (isOwnSock_ && socket_handle_valid(sock_)) {
        socket_close(sock_);
        sock_ = -1;
    }
}

UdpSocket::~UdpSocket() {
    close();
}

#endif // HAL_USE_SOCKET_HAL_POSIX

namespace ntp {

int parseResponse(uint64_t* timestamp, Message& msg, size_t size);

const char NTP_STRFTIME_FORMAT[] = "%Y-%m-%dT%H:%M:%S";
void dumpNtpTime(uint64_t timestamp);

} // ntp

uint64_t getRtcTime() {
    struct timeval tv = {};
    hal_rtc_get_time(&tv, nullptr);
    uint64_t unixtime = tv.tv_sec * ntp::USEC_IN_SEC;
    unixtime += tv.tv_usec;
    return unixtime;
}

SimpleNtpClient::SimpleNtpClient()
        : SimpleNtpClient(-1) {
}

SimpleNtpClient::SimpleNtpClient(sock_handle_t sock)
        : sock_{std::make_unique<UdpSocket>(sock)} {
}

SimpleNtpClient::~SimpleNtpClient() = default;

int SimpleNtpClient::ntpDate(uint64_t* timestamp, const char* hostname, system_tick_t timeout) {
    if (!hostname) {
        hostname = ntp::DEFAULT_SERVER;
    }

    CHECK(sock_->connect(hostname, ntp::PORT));

    const auto retries = std::max<unsigned>(timeout / ntp::RETRY_INTERVAL, 1);
    int lastError = SYSTEM_ERROR_UNKNOWN;
    // TODO: adjust the time based on the number of clock cycles
    for (unsigned retry = 0; retry < retries; retry++) {
        // Construct a request
        ntp::Message msg = {};
        msg.liVnMode = ntp::MODE_CLIENT | ntp::VERSION;
        msg.originTimestamp = ntp::Timestamp::fromUnixtime(getRtcTime());

        LOG_DEBUG(TRACE, "Sending request");
        CHECK(sock_->send((const uint8_t*)&msg, sizeof(msg)));

        LOG_DEBUG(TRACE, "Waiting for response");
        auto r = sock_->recv((uint8_t*)&msg, sizeof(msg), ntp::RETRY_INTERVAL);
        LOG_DEBUG(TRACE, "Response %ld", r);
        if (r < 0) {
            if (r == SYSTEM_ERROR_TIMEOUT) {
                lastError = r;
                continue;
            }

            return r;
        }

        lastError = ntp::parseResponse(timestamp, msg, r);
        if (!lastError) {
            break;
        } else {
            LOG(WARN, "Failed to parse NTP response");
        }
    }

    return lastError;
}

void ntp::dumpNtpTime(uint64_t timestamp) {
    time_t unixTime = timestamp / USEC_IN_SEC;
    unsigned usecs = timestamp - unixTime * USEC_IN_SEC;

    char buf[64] = {};
#if HAL_PLATFORM_GEN >= 3
    struct tm tm = {};
    localtime_r(&unixTime, &tm);
    char format[32] = {};
    snprintf(format, sizeof(format), "%s.%uZ", NTP_STRFTIME_FORMAT, usecs);
    std::strftime(buf, sizeof(buf), format, &tm);
#else
    // We don't have much flash space on certain platforms, so we opt not
    // to pull in strftime unnecessarily just to log the time.
    snprintf(buf, sizeof(buf), "%u.%u", (unsigned)unixTime, usecs);
#endif // HAL_PLATFORM_GEN >= 3
    LOG(TRACE, "NTP time: %s", buf);
}

int ntp::parseResponse(uint64_t* timestamp, ntp::Message& msg, size_t size) {
    CHECK_TRUE(size == sizeof(msg), SYSTEM_ERROR_BAD_DATA);
    CHECK_TRUE((msg.liVnMode & ntp::MODE_MASK) == ntp::MODE_SERVER, SYSTEM_ERROR_BAD_DATA);
    CHECK_TRUE(msg.stratum != ntp::STRATUM_KOD, SYSTEM_ERROR_INVALID_STATE);

    uint64_t ts = msg.destinationTimestamp.toUnixtime();
    dumpNtpTime(ts);

    if (timestamp) {
        *timestamp = ts;
    }

    return 0;
}

} // particle
