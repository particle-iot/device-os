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
#include "socket_hal_compat.h"
#include "inet_hal_compat.h"
#include "rtc_hal.h"
#include "rng_hal.h"
#include "simple_ntp_client_detail.h"

#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>
#include <hippomocks.h>

using namespace particle;

namespace {

const time_t DEFAULT_UNIXTIME = 1514764800; // 2018/01/01 00:00:00;
const uint32_t NTP_TIME_SECONDS = 0xffaad2dc; // May 26, 2017 13:24:15.887407901 UTC
const uint32_t NTP_TIME_FRACTION = 0x0a2a2de3; // May 26, 2017 13:24:15.887407901 UTC
const uint64_t NTP_TIME_UNIX_US = 1495805055ULL * 1000000 + 887407; // May 26, 2017 13:24:15.887407901 UTC
const uint32_t RESOLVED_IP = 0x11223344;

class SimpleNtpClientMocks {
public:
    SimpleNtpClientMocks()
            : sockAddr_{},
              socketAf_{AF_UNSPEC},
              enableResponse_{true},
              requestsReceived_{0} {

        mocks_.OnCallFunc(socket_create).Do([&](uint8_t family, uint8_t type, uint8_t protocol, uint16_t port, network_interface_t nif) {
            CHECK(socketAf_ == AF_UNSPEC);
            CHECK(type == SOCK_DGRAM);
            CHECK(protocol == IPPROTO_UDP);
            socketAf_ = family;
            return 123;
        });

        mocks_.OnCallFunc(socket_close).Do([&](sock_handle_t sock) {
            CHECK(!socketClosed());
            if (sock == 123) {
                socketAf_ = AF_UNSPEC;
                return 0;
            }
            return -1;
        });

        mocks_.OnCallFunc(inet_gethostbyname).Do([&](const char* hostname, uint16_t hostnameLen, HAL_IPAddress* out_ip_addr, network_interface_t nif, void* reserved) {
            CHECK(out_ip_addr);
            CHECK(strlen(hostname) == hostnameLen);
            out_ip_addr->ipv4 = RESOLVED_IP;

            memset(&sockAddr_, 0, sizeof(sockAddr_));
            sockAddr_.sa_family = AF_INET;
            sockAddr_.sa_data[0] = (ntp::PORT >> 8) & 0xff;
            sockAddr_.sa_data[1] = (ntp::PORT & 0xff);
            sockAddr_.sa_data[2] = (RESOLVED_IP >> 24) & 0xff;
            sockAddr_.sa_data[3] = (RESOLVED_IP >> 16) & 0xff;
            sockAddr_.sa_data[4] = (RESOLVED_IP >> 8) & 0xff;
            sockAddr_.sa_data[5] = (RESOLVED_IP) & 0xff;
            return 0;
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
        mocks_.OnCallFunc(socket_sendto).Do([&](sock_handle_t sd, const void* buffer, socklen_t len, uint32_t flags, sockaddr_t* addr, socklen_t addr_size) {
            CHECK(len == sizeof(request_));
            CHECK(addr_size >= sizeof(sockaddr_t));
            CHECK(addr);
            CHECK(!memcmp(addr, &sockAddr_, sizeof(sockAddr_)));

            memset(&request_, 0, sizeof(request_));
            memcpy(&request_, buffer, len);
            requestsReceived_++;
            return len;
        });

        mocks_.OnCallFunc(socket_receivefrom_ex).Do([&](sock_handle_t sock, void* buffer, socklen_t bufLen, uint32_t flags, sockaddr_t* addr, socklen_t* addrsize, system_tick_t timeout, void* reserved) -> sock_result_t {
            if (enableResponse_) {
                memset(&response_, 0, sizeof(response_));
                response_.liVnMode = ntp::MODE_SERVER | request_.liVnMode & (ntp::VERSION_MASK | ntp::LI_MASK);
                response_.stratum = 1;
                response_.refTime = request_.originTimestamp;
                response_.destinationTimestamp.seconds = NTP_TIME_SECONDS;
                response_.destinationTimestamp.fraction = NTP_TIME_FRACTION;
                response_.originTimestamp = response_.destinationTimestamp;
                response_.receiveTimestamp = response_.destinationTimestamp;
                memcpy(buffer, &response_, std::min<size_t>(bufLen, sizeof(response_)));
                if (addr && addrsize && *addrsize >= sizeof(sockAddr_)) {
                    *addr = sockAddr_;
                    *addrsize = sizeof(sockAddr_);
                }
                return sizeof(response_);
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
    sockaddr_t sockAddr_;
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
        mocks.NeverCallFunc(socket_close).Return(0);

        {
            SimpleNtpClient client;
        }
    }

    SECTION("construct with existing socket handle") {
        MockRepository mocks;
        mocks.NeverCallFunc(socket_close).With(123).Return(0);

        {
            SimpleNtpClient client(123);
        }
    }

    SECTION("DNS resolution errors correctly handled") {
        MockRepository mocks;

        mocks.ExpectCallFunc(inet_gethostbyname).Do([&](const char* hostname, uint16_t hostnameLen, HAL_IPAddress* out_ip_addr, network_interface_t nif, void* reserved) {
            return -1;
        });

        SimpleNtpClient client;
        CHECK(client.ntpDate(nullptr, "test", 1000) == SYSTEM_ERROR_NOT_FOUND);
    }

    SECTION("pool.ntp.org is used by default") {
        MockRepository mocks;

        mocks.ExpectCallFunc(inet_gethostbyname).Do([&](const char* hostname, uint16_t hostnameLen, HAL_IPAddress* out_ip_addr, network_interface_t nif, void* reserved) {
            CHECK(!strcmp(hostname, ntp::DEFAULT_SERVER));
            CHECK(strlen(hostname) == hostnameLen);
            return -1;
        });

        SimpleNtpClient client;
        CHECK(client.ntpDate(nullptr) == SYSTEM_ERROR_NOT_FOUND);
    }

    SECTION("custom NTP server can be supplied") {
        MockRepository mocks;

        mocks.ExpectCallFunc(inet_gethostbyname).Do([&](const char* hostname, uint16_t hostnameLen, HAL_IPAddress* out_ip_addr, network_interface_t nif, void* reserved) {
            CHECK(!strcmp(hostname, "test.ntp.server"));
            CHECK(strlen(hostname) == hostnameLen);
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
    }
}
