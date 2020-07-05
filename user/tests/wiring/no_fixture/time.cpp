/**
 ******************************************************************************
 * @file    time.cpp
 * @authors Satish Nair
 * @version V1.0.0
 * @date    7-Oct-2014
 * @brief   TIME test application
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "application.h"
#include "unit-test/unit-test.h"
#include "rtc_hal.h"
#include "simple_ntp_client.h"

test(TIME_01_NowReturnsCorrectUnixTime) {
    // when
    time_t last_time = Time.now();
    delay(1000); //systick delay for 1 second
    // then
    time_t current_time = Time.now();
    assertEqual(current_time, last_time + 1);//RTC interrupt fires successfully
}

test(TIME_02_LocalReturnsUnixTimePlusTimezone) {
    // when
	Time.zone(-5);
    Time.endDST();
	// Time.now() and Time.local();
    time_t last_time;
    time_t local_time;
    ATOMIC_BLOCK() {
        last_time = Time.now();
        local_time = Time.local();
    }
    // then
    assertEqual(local_time, last_time - (5*3600));
}

test(TIME_03_LocalReturnsUnixTimePlusTimezoneAndDST) {
    // when
    Time.zone(-5);
    Time.setDSTOffset(1.0);
    Time.beginDST();
    assertTrue(Time.isDST());
    // Time.now() and Time.local();
    time_t last_time;
    time_t local_time;
    ATOMIC_BLOCK() {
        last_time = Time.now();
        local_time = Time.local();
    }
    // then
    assertEqual(local_time, last_time - (4*3600));

    Time.endDST();
    assertFalse(Time.isDST());
}

test(TIME_04_LocalReturnsUnixTimePlusDST) {
    // when
    Time.zone(0);
    Time.setDSTOffset(1.25);
    Time.beginDST();
    assertTrue(Time.isDST());
    // Time.now() and Time.local();
    time_t last_time;
    time_t local_time;
    ATOMIC_BLOCK() {
        last_time = Time.now();
        local_time = Time.local();
    }
    // then
    assertEqual(local_time, last_time + (4500));

    Time.endDST();
    assertFalse(Time.isDST());
}

test(TIME_05_zoneIsReturned) {
	Time.zone(-10);
	assertEqual(Time.zone(), -10);
}

test(TIME_06_DSTOffsetIsReturned) {
    Time.setDSTOffset(1.5);
    assertEqual(Time.getDSTOffset(), 1.5);
}

test(TIME_07_SetTimeResultsInCorrectUnixTimeUpdate) {
    // when
    time_t current_time = Time.now();
    Time.setTime(1514764800);//set to 2018/01/01 00:00:00
    // then
    time_t temp_time = Time.now();
    assertEqual(temp_time, 1514764800);
    // restore original time
    Time.setTime(current_time);
}

test(TIME_08_TimeStrDoesNotEndWithNewline) {
    String t = Time.timeStr();
    assertMore(t.length(), 0);
    char c = t[t.length()-1];
    assertNotEqual('\n', c);
}

test(TIME_09_ChangingTimeZoneWorksImmediately) {
    for (int x=-12; x<=13; x++)
    {
        Time.zone(x);
        int currentHour = Time.hour();
        int y = x + 4;
        if (y > 13) {
            y -= 24;
        }
        Time.zone(y);
        int newHour = Time.hour();
        // check 24 hour wrapping case
        int diff = newHour - currentHour;
        if (diff < 0) {
            diff += 24;
        }
        assertEqual(diff, 4);
        Time.zone(x);
        newHour = Time.hour();
        assertEqual((newHour-currentHour), 0);
    }
}

test(TIME_10_ChangingDSTWorksImmediately) {
    for (float x=-12; x<=13; x+=0.5)
    {
        Time.zone(x);
        int currentHour = Time.hour();
        Time.setDSTOffset(1.0);
        Time.beginDST();
        assertTrue(Time.isDST());
        int newHour = Time.hour();
        // check 24 hour wrapping case
        int diff = newHour - currentHour;
        if (diff < 0) {
            diff += 24;
        }
        assertEqual(diff, 1);
        Time.endDST();
        newHour = Time.hour();
        assertEqual((newHour-currentHour), 0);
        assertFalse(Time.isDST());
    }
}

test(TIME_11_Format) {
    Time.endDST();
    Time.zone(-5.25);
    time_t t = 1024*1024*1024;
    assertEqual(Time.timeStr(t).c_str(),(const char*)"Sat Jan 10 08:22:04 2004");
    assertEqual(Time.format(t, TIME_FORMAT_DEFAULT).c_str(), (const char*)("Sat Jan 10 08:22:04 2004"));
    assertEqual(Time.format(t, TIME_FORMAT_ISO8601_FULL).c_str(), (const char*)"2004-01-10T08:22:04-05:15");
    Time.setFormat(TIME_FORMAT_ISO8601_FULL);
    assertEqual(Time.format(t).c_str(), (const char*)("2004-01-10T08:22:04-05:15"));
    Time.zone(0);
    assertEqual(Time.format(t).c_str(), (const char*)("2004-01-10T13:37:04Z"));
    Time.setFormat(TIME_FORMAT_DEFAULT);

    Time.setDSTOffset(1.5);
    Time.zone(-5.25);
    Time.beginDST();
    assertTrue(Time.isDST());
    t = 1024*1024*1024;
    assertEqual(Time.timeStr(t).c_str(),(const char*)"Sat Jan 10 09:52:04 2004");
    assertEqual(Time.format(t, TIME_FORMAT_DEFAULT).c_str(), (const char*)("Sat Jan 10 09:52:04 2004"));
    assertEqual(Time.format(t, TIME_FORMAT_ISO8601_FULL).c_str(), (const char*)"2004-01-10T09:52:04-03:45");
    Time.setFormat(TIME_FORMAT_ISO8601_FULL);
    assertEqual(Time.format(t).c_str(), (const char*)("2004-01-10T09:52:04-03:45"));
    Time.zone(0);
    assertEqual(Time.format(t).c_str(), (const char*)("2004-01-10T15:07:04+01:30"));
    Time.setFormat(TIME_FORMAT_DEFAULT);
    Time.endDST();
    assertFalse(Time.isDST());
}

test(TIME_12_concatenate) {
    // addresses reports of timeStr() not being concatenatable
    time_t t = 1024*1024*1024;
    Time.endDST();
    Time.zone(0);
    assertEqual(Time.timeStr(t).c_str(),(const char*)"Sat Jan 10 13:37:04 2004");
    String s = Time.timeStr(t);
    s += "abcd";
    assertEqual(s.c_str(), (const char*)"Sat Jan 10 13:37:04 2004abcd");
}

test(TIME_13_syncTimePending_syncTimeDone_when_disconnected)
{
    if (!Particle.connected())
    {
        Particle.connect();
        waitFor(Particle.connected, 120000);
    }
    assertTrue(Particle.connected());
    Particle.syncTime();
    Particle.disconnect();
    waitFor(Particle.disconnected, 10000);
    assertTrue(Particle.disconnected());

    assertTrue(Particle.syncTimeDone());
    assertFalse(Particle.syncTimePending());
}

test(TIME_14_timeSyncedLast_works_correctly)
{
    if (!Particle.connected())
    {
        Particle.connect();
        waitFor(Particle.connected, 120000);
    }
    uint32_t mil = millis();
    Particle.syncTime();
    waitFor(Particle.syncTimeDone, 120000);
    assertMore(Particle.timeSyncedLast(), mil);
}

test(TIME_15_RestoreSystemMode) {
    set_system_mode(AUTOMATIC);
    if (!Particle.connected()) {
        Particle.connect();
        waitFor(Particle.connected, 120000);
    }
}

static int s_time_changed_reason = -1;
static void time_changed_handler(system_event_t event, int param)
{
    s_time_changed_reason = param;
}

test(TIME_16_TimeChangedEvent) {
    assertTrue(Particle.connected());

    system_tick_t syncedLastMillis = Particle.timeSyncedLast();

    System.on(time_changed, time_changed_handler);
    Time.setTime(1514764800);
    assertEqual(s_time_changed_reason, (int)time_changed_manually);
    s_time_changed_reason = -1;

    Particle.syncTime();
    waitFor(Particle.syncTimeDone, 120000);
    // If wiring/no_fixture was built with USE_THREADING=y, we need to process application queue here
    // in order to ensure that event handler has been called by the time we check s_time_changed_reason
    Particle.process();
    assertMore(Particle.timeSyncedLast(), syncedLastMillis);
    assertEqual(s_time_changed_reason, (int)time_changed_sync);
}

test(TIME_17_RtcAlarmFiresCorrectly) {
    if (Particle.syncTimePending()) {
        waitFor(Particle.syncTimeDone, 60000);
        assertTrue(Particle.syncTimeDone());
        assertTrue(Time.isValid());
    }

    // Absolute time
    struct timeval now;
    assertEqual(0, hal_rtc_get_time(&now, nullptr));
    now.tv_sec += 5;
    static volatile bool alarmFired = false;
    auto ms = millis();
    int r = hal_rtc_set_alarm(&now, 0, [](void* ctx) -> void {
        volatile bool* alarmFired = (volatile bool*)ctx;
        *alarmFired = true;
    }, (void*)&alarmFired, nullptr);
    assertEqual(r, 0);
    while (!alarmFired && (millis() - ms) <= 6000) {
        delay(1);
    }
    assertLessOrEqual(millis() - ms, 6000);
    hal_rtc_cancel_alarm();
    assertTrue((bool)alarmFired);

    // In some amount of time
    alarmFired = false;
    now.tv_sec = 5;
    now.tv_usec = 0;
    ms = millis();
    r = hal_rtc_set_alarm(&now, HAL_RTC_ALARM_FLAG_IN, [](void* ctx) -> void {
        volatile bool* alarmFired = (volatile bool*)ctx;
        *alarmFired = true;
    }, (void*)&alarmFired, nullptr);
    assertEqual(r, 0);
    while (!alarmFired && (millis() - ms) <= 6000) {
        delay(1);
    }
    assertLessOrEqual(millis() - ms, 6000);
    hal_rtc_cancel_alarm();
    assertTrue((bool)alarmFired);
}

test(TIME_18_RtcAlarmReturnsAnErrorWhenTimeInThePast) {
    if (Particle.syncTimePending()) {
        waitFor(Particle.syncTimeDone, 60000);
        assertTrue(Particle.syncTimeDone());
        assertTrue(Time.isValid());
    }

    // Absolute time
    struct timeval now;
    assertEqual(0, hal_rtc_get_time(&now, nullptr));
    now.tv_sec -= 60;
    static volatile bool alarmFired = false;
    int r = hal_rtc_set_alarm(&now, 0, [](void* ctx) -> void {
        volatile bool* alarmFired = (volatile bool*)ctx;
        *alarmFired = true;
    }, (void*)&alarmFired, nullptr);
    hal_rtc_cancel_alarm();
    assertFalse((bool)alarmFired);
    assertEqual(r, (int)SYSTEM_ERROR_TIMEOUT);

    // In some amount of time
    alarmFired = false;
    now.tv_sec = -5;
    now.tv_usec = 0;
    r = hal_rtc_set_alarm(&now, HAL_RTC_ALARM_FLAG_IN, [](void* ctx) -> void {
        volatile bool* alarmFired = (volatile bool*)ctx;
        *alarmFired = true;
    }, (void*)&alarmFired, nullptr);
    hal_rtc_cancel_alarm();
    assertFalse((bool)alarmFired);
    assertEqual(r, (int)SYSTEM_ERROR_TIMEOUT);
}

test(TIME_19_LocalTimeIsCloseToNtpTime) {
    auto client = std::make_unique<SimpleNtpClient>();

    assertTrue((bool)client);

    assertTrue(Particle.connected());
    Particle.syncTime();
    waitFor(Particle.syncTimeDone, 60000);
    assertTrue(Particle.syncTimeDone());
    assertTrue(Time.isValid());

    uint64_t ntpTime = 0;
    assertEqual(0, client->ntpDate(&ntpTime));

    struct timeval tv = {};
    assertEqual(0, hal_rtc_get_time(&tv, nullptr));
    uint64_t now = tv.tv_sec * 1000000ULL + tv.tv_usec;

    // Within 10 seconds
    const int64_t diff = std::chrono::microseconds(10s).count();
    assertLessOrEqual(std::abs((int64_t)now - (int64_t)ntpTime), diff);
}
