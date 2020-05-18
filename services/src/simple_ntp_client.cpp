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

LOG_SOURCE_CATEGORY("serivce.ntp");

#include "hal_platform.h"

#include "simple_ntp_client.h"
#include "check.h"

#include "inet_hal.h"

#if HAL_USE_SOCKET_HAL_POSIX
#include "netdb_hal.h"
#include <arpa/inet.h>
#else
#include "inet_hal_compat.h"
#endif // HAL_USE_SOCKET_HAL_POSIX

#include "random.h"
#include "system_tick_hal.h"
#include "rtc_hal.h"
#include <ctime>
#include "timer_hal.h"

namespace particle {

class UdpSocket {
public:
    UdpSocket() :
            UdpSocket(-1) {
    }

    UdpSocket(sock_handle_t sock)
            : sock_{sock},
              close_{sock < 0 ? true : false},
#if HAL_USE_SOCKET_HAL_POSIX
              addr_{nullptr},
              cached_{nullptr} {
#else
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
    bool close_;

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
        CHECK_TRUE(netdb_getaddrinfo(hostname, tmpserv, &hints, &addr_) == 0 && addr_, SYSTEM_ERROR_NETWORK);
        cached_ = addr_;
    }

    int sock = -1;
    for (; cached_ != nullptr && sock < 0; cached_ = cached_->ai_next) {
        /* Iterate over all the addresses and attempt to connect */

        int s = sock_socket(cached_->ai_family, cached_->ai_socktype, cached_->ai_protocol);
        if (s < 0) {
            continue;
        }

        LOG(TRACE, "SNTP socket=%d, family=%d, type=%d, protocol=%d", s, cached_->ai_family, cached_->ai_socktype, cached_->ai_protocol);

        char serverHost[INET6_ADDRSTRLEN] = {};
        uint16_t serverPort = 0;
        switch (cached_->ai_family) {
            case AF_INET: {
                inet_inet_ntop(cached_->ai_family, &((sockaddr_in*)cached_->ai_addr)->sin_addr, serverHost, sizeof(serverHost));
                serverPort = ntohs(((sockaddr_in*)cached_->ai_addr)->sin_port);
                break;
            }
            case AF_INET6: {
                inet_inet_ntop(cached_->ai_family, &((sockaddr_in6*)cached_->ai_addr)->sin6_addr, serverHost, sizeof(serverHost));
                serverPort = ntohs(((sockaddr_in6*)cached_->ai_addr)->sin6_port);
                break;
            }
        }
        LOG(TRACE, "SNTP socket=%d, connecting to %s#%u", s, serverHost, serverPort);

        int r = sock_connect(s, cached_->ai_addr, cached_->ai_addrlen);
        if (r) {
            sock_close(s);
            continue;
        }

        sock = s;
        LOG(TRACE, "SNTP connected");
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
    if (close_) {
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

int UdpSocket::connect(const char* hostname) {
}

ssize_t UdpSocket::recv(uint8_t* buf, size_t size, system_tick_t timeout) {
}

ssize_t UdpSocket::send(const uint8_t* buf, size_t size) {
}

void UdpSocket::close() {
    if (close_) {
        sock_close(sock_);
        sock_ = -1;
    }
}

UdpSocket::~UdpSocket() {
    close();
}

#endif // HAL_USE_SOCKET_HAL_POSIX

namespace ntp {

const char DEFAULT_SERVER[] = "pool.ntp.org";
const uint16_t PORT = 123;
const system_tick_t RETRY_INTERVAL = 2000;

const uint8_t LI_MASK = 0x03 << 6;
const uint8_t VERSION_MASK = 0x07 << 3;
const uint8_t MODE_MASK = 0x07;
const uint8_t VERSION = 4 << 3;

enum Li {
    LI_NO_WARNING = 0 << 6,
    LI_LAST_MINUTE_61 = 1 << 6,
    LI_LAST_MINUTE_59 = 2 << 6,
    LI_ALARM = 3 << 6
};

enum Mode {
    MODE_RESERVED = 0,
    MODE_SYMMETRIC_ACTIVE = 1,
    MODE_SYMMETRIC_PASSIVE = 2,
    MODE_CLIENT = 3,
    MODE_SERVER = 4,
    MODE_BROADCAST = 5,
    MODE_RESERVED_CONTROL_MESSAGE = 6,
    MODE_RESERVED_PRIVATE = 7,
};

enum Stratum {
    STRATUM_KOD = 0
};

const uint32_t SECONDS_FROM_1970_TO_1900 = 2208988800UL;
const uint32_t USEC_IN_SEC = 1000000;
const uint32_t FRACTIONS_IN_SEC_POW = 32;

struct Timestamp {
    uint32_t seconds;
    uint32_t fraction;

    static Timestamp fromUnixtime(uint64_t unixMicros) {
        uint32_t seconds = unixMicros / USEC_IN_SEC;
        uint32_t frac = ((unixMicros - seconds) << FRACTIONS_IN_SEC_POW) / USEC_IN_SEC;
        particle::Random rand;
        rand.gen((char*)&frac, sizeof(char));
        return {htonl(seconds + SECONDS_FROM_1970_TO_1900), htonl(frac)};
    }

    uint64_t toUnixtime() const {
        uint64_t unixMicros = (uint64_t)(ntohl(seconds) - SECONDS_FROM_1970_TO_1900) * USEC_IN_SEC;
        uint64_t frac = (((uint64_t)ntohl(fraction)) * USEC_IN_SEC) >> FRACTIONS_IN_SEC_POW;
        return unixMicros + frac;
    }
} __attribute__((packed));

struct Message {
    uint8_t liVnMode; // LI + VN + mode
    uint8_t stratum; // stratum
    uint8_t poll; // poll exponent
    uint8_t precision; // precision exponent
    uint32_t rootDelay; // rootdelay
    uint32_t rootDisp; // rootdisp
    uint32_t refId; // refid
    Timestamp refTime; // reftime
    Timestamp originTimestamp; // org
    Timestamp receiveTimestamp; // rec
    Timestamp destinationTimestamp; // dst
    // uint32_t keyId; // key identifier
    // uint8_t digest[16]; // message digest
} __attribute__((packed));

int parseResponse(uint64_t* timestamp, Message& msg, size_t size);

const char NTP_STRFTIME_FORMAT[] = "%Y-%m-%dT%H:%M:%S";
void dumpNtpTime(uint64_t timestamp);

} // ntp

uint64_t getRtcTime() {
    uint64_t unixtime = HAL_RTC_Get_UnixTime() * ntp::USEC_IN_SEC;
    unixtime += hal_timer_micros(nullptr) % ntp::USEC_IN_SEC;
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
    for (unsigned retry = 0; retry < retries; retry++) {
        // Construct a request
        ntp::Message msg = {};
        msg.liVnMode = ntp::MODE_CLIENT | ntp::VERSION;
        msg.originTimestamp = ntp::Timestamp::fromUnixtime(getRtcTime());

        LOG(TRACE, "Sending request");
        lastError = sock_->send((const uint8_t*)&msg, sizeof(msg));
        if (lastError < 0) {
            CHECK(sock_->connect(hostname, ntp::PORT));
        }

        LOG(TRACE, "Waiting for response");
        auto r = sock_->recv((uint8_t*)&msg, sizeof(msg), ntp::RETRY_INTERVAL);
        LOG(TRACE, "Response %ld", r);
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
        }
    }

    return lastError;
}

void ntp::dumpNtpTime(uint64_t timestamp) {
    time_t unixTime = timestamp / USEC_IN_SEC;
    struct tm tm = {};
    localtime_r(&unixTime, &tm);
    uint32_t usecs = timestamp - unixTime;

    char format[32] = {};
    char buf[64] = {};
    snprintf(format, sizeof(format), "%s.%luZ", NTP_STRFTIME_FORMAT, usecs);
    std::strftime(buf, sizeof(buf), format, &tm);
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
