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

#include "application.h"
#include "unit-test/unit-test.h"
#include "socket_hal.h"
#include "rtc_hal.h"
#include "scope_guard.h"

namespace {

bool tmEqual(const struct tm& lh, const struct tm& rh) {
    return lh.tm_sec == rh.tm_sec &&
        lh.tm_min == rh.tm_min &&
        lh.tm_hour == rh.tm_hour &&
        lh.tm_mday == rh.tm_mday &&
        lh.tm_mon == rh.tm_mon &&
        lh.tm_year == rh.tm_year &&
        lh.tm_wday == rh.tm_wday &&
        lh.tm_yday == rh.tm_yday &&
        lh.tm_isdst == rh.tm_isdst;
}

} // anonymous

STARTUP({
    Log.trace("Current time: %s", Time.timeStr().c_str());
});

test(TIME_COMPAT_00_TimeIsValid) {
    Particle.connect();
    waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
    assertTrue(Particle.connected());
    if (Particle.syncTimePending()) {
        waitFor(Particle.syncTimeDone, 120000);
        assertTrue(Particle.syncTimeDone());
    }
    assertTrue(Time.isValid());
}

#if HAL_USE_SOCKET_HAL_POSIX
test(TIME_COMPAT_01_SocketSelect) {
    int s = sock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SCOPE_GUARD({
        if (s >= 0) {
            sock_close(s);
        }
    });
    assertMoreOrEqual(s, 0);

    struct sockaddr_in sin = {};
    sin.sin_family = AF_INET;
    sin.sin_port = 0x1122;

    assertEqual(0, sock_bind(s, (const sockaddr*)&sin, sizeof(sin)));

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(s, &readfds);
    struct timeval tv = {
        .tv_sec = 2,
        .tv_usec = 0
    };

    auto ms = millis();
    assertEqual(0, sock_select(s + 1, &readfds, nullptr, nullptr, &tv));
    assertMoreOrEqual(millis() - ms, 1990);

    struct timeval32 tv32 = {
        .tv_sec = 2,
        .tv_usec = 0
    };
    uint32_t garbage = 0xffffffff;
    (void)garbage;
    ms = millis();
    assertEqual(0, sock_select32(s + 1, &readfds, nullptr, nullptr, &tv32));
    assertMoreOrEqual(millis() - ms, 1990);
}

test(TIME_COMPAT_02_SocketSetGetSockOptRcvTimeo) {
    int s = sock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SCOPE_GUARD({
        if (s >= 0) {
            sock_close(s);
        }
    });
    assertMoreOrEqual(s, 0);

    struct sockaddr_in sin = {};
    sin.sin_family = AF_INET;
    sin.sin_port = 0x1122;

    assertEqual(0, sock_bind(s, (const sockaddr*)&sin, sizeof(sin)));

    struct timeval tv = {
        .tv_sec = 0xbad,
        .tv_usec = 0
    };
    assertEqual(0, sock_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)));

    socklen_t size = sizeof(tv);
    memset(&tv, 0, sizeof(tv));
    assertEqual(0, sock_getsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, &size));
    assertEqual(size, sizeof(tv));
    assertEqual(tv.tv_sec, 0xbad);
    assertEqual(tv.tv_usec, 0);

    struct timeval32 tv32 = {};
    uint32_t garbage = 0xffffffff;
    (void)garbage;
    size = sizeof(tv32);
    assertEqual(0, sock_getsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv32, &size));
    assertEqual(size, sizeof(tv32));
    assertEqual(tv32.tv_sec, 0xbad);
    assertEqual(tv32.tv_usec, 0);
    assertEqual(garbage, 0xffffffff);

    tv32.tv_sec = 0xbeef;
    tv32.tv_usec = 0;
    assertEqual(0, sock_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv32, sizeof(tv32)));

    size = sizeof(tv32);
    memset(&tv32, 0, sizeof(tv32));
    assertEqual(0, sock_getsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv32, &size));
    assertEqual(size, sizeof(tv32));
    assertEqual(tv32.tv_sec, 0xbeef);
    assertEqual(tv32.tv_usec, 0);
    assertEqual(garbage, 0xffffffff);
}

test(TIME_COMPAT_03_SocketSetGetSockOptSndTimeo) {
    int s = sock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SCOPE_GUARD({
        if (s >= 0) {
            sock_close(s);
        }
    });
    assertMoreOrEqual(s, 0);

    struct timeval tv = {
        .tv_sec = 0xbad,
        .tv_usec = 0
    };
    assertEqual(0, sock_setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)));

    socklen_t size = sizeof(tv);
    memset(&tv, 0, sizeof(tv));
    assertEqual(0, sock_getsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, &size));
    assertEqual(size, sizeof(tv));
    assertEqual(tv.tv_sec, 0xbad);
    assertEqual(tv.tv_usec, 0);

    struct timeval32 tv32 = {};
    uint32_t garbage = 0xffffffff;
    (void)garbage;
    size = sizeof(tv32);
    assertEqual(0, sock_getsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv32, &size));
    assertEqual(size, sizeof(tv32));
    assertEqual(tv32.tv_sec, 0xbad);
    assertEqual(tv32.tv_usec, 0);
    assertEqual(garbage, 0xffffffff);

    tv32.tv_sec = 0xbeef;
    tv32.tv_usec = 0;
    assertEqual(0, sock_setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv32, sizeof(tv32)));

    size = sizeof(tv32);
    memset(&tv32, 0, sizeof(tv32));
    assertEqual(0, sock_getsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv32, &size));
    assertEqual(size, sizeof(tv32));
    assertEqual(tv32.tv_sec, 0xbeef);
    assertEqual(tv32.tv_usec, 0);
    assertEqual(garbage, 0xffffffff);
}

#endif // HAL_USE_SOCKET_HAL_POSIX

test(TIME_COMPAT_04_RtcHal) {
    time_t refTime = 1577836800; // 2020-01-01 00:00:00

    struct timeval tv = {
        .tv_sec = refTime,
        .tv_usec = 0
    };
    assertEqual(0, hal_rtc_set_time(&tv, nullptr));
    memset(&tv, 0, sizeof(tv));
    assertEqual(0, hal_rtc_get_time(&tv, nullptr));
    time32_t t = hal_rtc_get_unixtime_deprecated();

    assertMoreOrEqual(tv.tv_sec, refTime);
    assertLessOrEqual(tv.tv_sec - refTime, 1);
    assertMoreOrEqual(static_cast<time_t>(t), tv.tv_sec);
    assertLessOrEqual(static_cast<time_t>(t) - tv.tv_sec, 1);

    hal_rtc_set_unixtime_deprecated(static_cast<time32_t>(refTime));
    memset(&tv, 0, sizeof(tv));
    assertEqual(0, hal_rtc_get_time(&tv, nullptr));
    t = hal_rtc_get_unixtime_deprecated();

    assertMoreOrEqual(tv.tv_sec, refTime);
    assertLessOrEqual(tv.tv_sec - refTime, 1);
    assertMoreOrEqual(static_cast<time_t>(t), tv.tv_sec);
    assertLessOrEqual(static_cast<time_t>(t) - tv.tv_sec, 1);
}

test(TIME_COMPAT_05_SyncTime) {
    Particle.connect();
    waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME);
    assertTrue(Particle.connected());

    time_t refTime = 1546300800; // 2019-01-01 00:00:00

    struct timeval tv = {
        .tv_sec = refTime,
        .tv_usec = 0
    };
    assertEqual(0, hal_rtc_set_time(&tv, nullptr));
    memset(&tv, 0, sizeof(tv));
    assertEqual(0, hal_rtc_get_time(&tv, nullptr));
    assertMoreOrEqual(tv.tv_sec, refTime);
    assertLessOrEqual(tv.tv_sec - refTime, 1);

    auto ms = millis();
    Particle.syncTime();
    waitFor(Particle.syncTimeDone, 120000);
    assertTrue(Particle.syncTimeDone());

    assertTrue(Time.isValid());

    time32_t cloudTime32;
    time_t cloudTime;
    auto synced = spark_sync_time_last(&cloudTime32, &cloudTime);
    assertMoreOrEqual(synced, ms);
    assertEqual(static_cast<time_t>(cloudTime32), cloudTime);

    memset(&tv, 0, sizeof(tv));
    assertEqual(0, hal_rtc_get_time(&tv, nullptr));
    time32_t t = hal_rtc_get_unixtime_deprecated();
    assertMoreOrEqual(tv.tv_sec, cloudTime);
    assertMoreOrEqual(t, cloudTime32);
}
