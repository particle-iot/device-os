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

#include "simple_ntp_client.h"
#include "system_error.h"
#include "netdb_hal.h"
#include "socket_hal_posix.h"
#include "inet_hal_posix.h"
#include "simple_ntp_client_detail.h"
#include "rtc_hal.h"
#include "rng_hal.h"

#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>
#include <hippomocks.h>

using namespace particle;

namespace {

const time_t DEFAULT_UNIXTIME = 1514764800; // 2018/01/01 00:00:00;
const uint32_t NTP_TIME_SECONDS = 0xffaad2dc; // May 26, 2017 13:24:15.887407901 UTC
const uint32_t NTP_TIME_FRACTION = 0x0a2a2de3; // May 26, 2017 13:24:15.887407901 UTC
const uint64_t NTP_TIME_UNIX_US = 1495805055ULL * 1000000 + 887407; // May 26, 2017 13:24:15.887407901 UTC

class SimpleNtpClientMocks {
public:
    SimpleNtpClientMocks()
            : v4Addr_{},
              v4SockAddr_{},
              v6Addr_{},
              v6SockAddr_{},
              addrInfoAlloced_{nullptr},
              socketAf_{AF_UNSPEC},
              enableResponse_{true},
              requestsReceived_{0} {

        mocks_.OnCallFunc(sock_socket).Do([&](int domain, int type, int protocol) {
            CHECK(socketAf_ == AF_UNSPEC);
            CHECK((domain == AF_INET || domain == AF_INET6));
            CHECK(type == SOCK_DGRAM);
            CHECK(protocol == IPPROTO_UDP);
            socketAf_ = domain;
            return 123;
        });

        mocks_.OnCallFunc(sock_close).Do([&](int sock) {
            CHECK(!socketClosed());
            if (sock == 123) {
                socketAf_ = AF_UNSPEC;
                return 0;
            }
            return -1;
        });

        mocks_.OnCallFunc(netdb_getaddrinfo).Do([&](const char* hostname, const char* servname, const struct addrinfo* hints, struct addrinfo** res) {
            CHECK(addrInfoAlloced_ == nullptr);

            if (!strcmp(hostname, "ipv4")) {
                v4SockAddr_.sin_family = AF_INET;
                v4SockAddr_.sin_port = 123;
                v4SockAddr_.sin_addr.s_addr = htonl(0x11223344);
                v4Addr_.ai_flags = hints->ai_flags;
                v4Addr_.ai_family = AF_INET;
                v4Addr_.ai_socktype = hints->ai_socktype;
                v4Addr_.ai_protocol = hints->ai_protocol;
                v4Addr_.ai_addrlen = sizeof(v4SockAddr_);
                v4Addr_.ai_addr = reinterpret_cast<struct sockaddr*>(&v4SockAddr_);
                v4Addr_.ai_canonname = nullptr;
                v4Addr_.ai_next = nullptr;
                *res = &v4Addr_;
                addrInfoAlloced_ = &v4Addr_;
                return 0;
            } else if (!strcmp(hostname, "ipv6")) {
                v6SockAddr_.sin6_family = AF_INET6;
                v6SockAddr_.sin6_port = 123;
                uint8_t tmp[] = {
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
                };
                memcpy(v6SockAddr_.sin6_addr.s6_addr, tmp, sizeof(v6SockAddr_.sin6_addr.s6_addr));
                v6Addr_.ai_flags = hints->ai_flags;
                v6Addr_.ai_family = AF_INET6;
                v6Addr_.ai_socktype = hints->ai_socktype;
                v6Addr_.ai_protocol = hints->ai_protocol;
                v6Addr_.ai_addrlen = sizeof(v6SockAddr_);
                v6Addr_.ai_addr = reinterpret_cast<struct sockaddr*>(&v6SockAddr_);
                v6Addr_.ai_canonname = nullptr;
                v6Addr_.ai_next = nullptr;
                *res = &v6Addr_;
                addrInfoAlloced_ = &v6Addr_;
                return 0;
            }

            return -1;
        });
        mocks_.OnCallFunc(netdb_freeaddrinfo).Do([&](struct addrinfo* ai) {
            CHECK(addrInfoAlloced_ != nullptr);
            if (ai && ai == addrInfoAlloced_) {
                addrInfoAlloced_ = nullptr;
            }
        });

        mocks_.OnCallFunc(HAL_RNG_GetRandomNumber).Return(12345);
        mocks_.OnCallFunc(hal_rtc_get_time).Do([&](struct timeval* tv, void* reserved) -> int {
            if (tv) {
                tv->tv_sec = DEFAULT_UNIXTIME;
                tv->tv_usec = 0;
                return 0;
            }
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        });
        mocks_.OnCallFunc(sock_setsockopt).Return(0);
        mocks_.OnCallFunc(sock_connect).With(123, (struct sockaddr*)&v4SockAddr_, sizeof(v4SockAddr_)).Return(0);
        mocks_.OnCallFunc(sock_connect).With(123, (struct sockaddr*)&v6SockAddr_, sizeof(v6SockAddr_)).Return(0);
        mocks_.OnCallFunc(sock_send).Do([&](int s, const void* dataptr, size_t size, int flags) {
            CHECK(size == sizeof(request_));
            memset(&request_, 0, sizeof(request_));
            memcpy(&request_, dataptr, size);
            requestsReceived_++;
            return size;
        });
        mocks_.OnCallFunc(sock_recv).Do([&](int s, void* dataptr, size_t size, int flags) -> ssize_t {
            if (enableResponse_) {
                memset(&response_, 0, sizeof(response_));
                response_.liVnMode = ntp::MODE_SERVER | request_.liVnMode & (ntp::VERSION_MASK | ntp::LI_MASK);
                response_.stratum = 1;
                response_.refTime = request_.originTimestamp;
                response_.destinationTimestamp.seconds = NTP_TIME_SECONDS;
                response_.destinationTimestamp.fraction = NTP_TIME_FRACTION;
                response_.originTimestamp = response_.destinationTimestamp;
                response_.receiveTimestamp = response_.destinationTimestamp;
                memcpy(dataptr, &response_, std::min(size, sizeof(response_)));
                return sizeof(response_);
            } else {
                errno = EAGAIN;
            }
            return -1;
        });
    }

    MockRepository& operator()() {
        return mocks_;
    }

    bool socketClosed() const {
        return socketAf_ == AF_UNSPEC;
    }

    bool addrInfoFreed() const {
        return addrInfoAlloced_ == nullptr;
    }

    auto request() const {
        return request_;
    }

    auto response() const {
        return response_;
    }

    void enableResponse(bool state) {
        enableResponse_ = state;
    }

    int requestsReceived() const {
        return requestsReceived_;
    }

private:
    MockRepository mocks_;
    struct addrinfo v4Addr_;
    struct sockaddr_in v4SockAddr_;
    struct addrinfo v6Addr_;
    struct sockaddr_in6 v6SockAddr_;
    struct addrinfo* addrInfoAlloced_;
    int socketAf_;
    bool enableResponse_;
    int requestsReceived_;

    ntp::Message request_;
    ntp::Message response_;
};

} // namespace

TEST_CASE("SimpleNtpClient") {
    SECTION("default constructor") {
        MockRepository mocks;
        mocks.NeverCallFunc(sock_close).Return(0);

        {
            SimpleNtpClient client;
        }
    }

    SECTION("construct with existing socket handle") {
        MockRepository mocks;
        mocks.NeverCallFunc(sock_close).With(123).Return(0);

        {
            SimpleNtpClient client(123);
        }
    }

    SECTION("DNS resolution errors correctly handled") {
        MockRepository mocks;

        mocks.ExpectCallFunc(netdb_getaddrinfo).Do([&](const char* hostname, const char* servname, const struct addrinfo* hints, struct addrinfo** res) {
            return -1;
        });

        SimpleNtpClient client;
        CHECK(client.ntpDate(nullptr, "test", 1000) == SYSTEM_ERROR_NOT_FOUND);
    }

    SECTION("pool.ntp.org is used by default") {
        MockRepository mocks;

        mocks.ExpectCallFunc(netdb_getaddrinfo).Do([&](const char* hostname, const char* servname, const struct addrinfo* hints, struct addrinfo** res) {
            CHECK(!strcmp(hostname, ntp::DEFAULT_SERVER));
            CHECK(!strcmp(servname, "123"));
            return -1;
        });

        SimpleNtpClient client;
        CHECK(client.ntpDate(nullptr) == SYSTEM_ERROR_NOT_FOUND);
    }

    SECTION("custom NTP server can be supplied") {
        MockRepository mocks;

        mocks.ExpectCallFunc(netdb_getaddrinfo).Do([&](const char* hostname, const char* servname, const struct addrinfo* hints, struct addrinfo** res) {
            CHECK(!strcmp(hostname, "test.ntp.server"));
            CHECK(!strcmp(servname, "123"));
            return -1;
        });

        SimpleNtpClient client;
        CHECK(client.ntpDate(nullptr, "test.ntp.server") == SYSTEM_ERROR_NOT_FOUND);
    }

    SECTION("acquires NTP time from IPv4 server") {
        SimpleNtpClientMocks mocks;

        {
            SimpleNtpClient client;
            uint64_t ntpTime = 0;
            CHECK(client.ntpDate(&ntpTime, "ipv4") == SYSTEM_ERROR_NONE);
            CHECK(ntpTime == NTP_TIME_UNIX_US);
        }
        CHECK(mocks.socketClosed());
        CHECK(mocks.addrInfoFreed());
    }

    SECTION("acquires NTP time from IPv6 server") {
        SimpleNtpClientMocks mocks;

        {
            SimpleNtpClient client;
            uint64_t ntpTime = 0;
            CHECK(client.ntpDate(&ntpTime, "ipv6") == SYSTEM_ERROR_NONE);
            CHECK(ntpTime == NTP_TIME_UNIX_US);
        }
        CHECK(mocks.socketClosed());
        CHECK(mocks.addrInfoFreed());
    }

    SECTION("retries a number of times until reaching a timeout") {
        SimpleNtpClientMocks mocks;

        {
            SimpleNtpClient client;
            uint64_t ntpTime = 0;
            mocks.enableResponse(false);
            CHECK(client.ntpDate(&ntpTime, "ipv4", 10000) == SYSTEM_ERROR_TIMEOUT);
            CHECK(mocks.requestsReceived() == (10000 / ntp::RETRY_INTERVAL));
        }
        CHECK(mocks.socketClosed());
        CHECK(mocks.addrInfoFreed());
    }
}
