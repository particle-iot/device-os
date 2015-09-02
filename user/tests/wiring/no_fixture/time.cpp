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

#if PLATFORM_ID>=3

#include "application.h"
#include "unit-test/unit-test.h"

test(TIME_NowReturnsCorrectUnixTime) {
    // when
    time_t last_time = Time.now();
    delay(1000); //systick delay for 1 second
    // then
    time_t current_time = Time.now();
    assertEqual(current_time, last_time + 1);//RTC interrupt fires successfully
}

test(TIME_SetTimeResultsInCorrectUnixTimeUpdate) {
    // when
    time_t current_time = Time.now();
    Time.setTime(86400);//set to epoch time + 1 day
    // then
    time_t temp_time = Time.now();
    assertEqual(temp_time, 86400);
    // restore original time
    Time.setTime(current_time);
}

test(TIME_TimeStrDoesNotEndWithNewline) {
    String t = Time.timeStr();
    assertMore(t.length(), 0);
    char c = t[t.length()-1];
    assertNotEqual('\n', c);
}

test(TIME_ChangingTimeZoneWorksImmediately) {
    int currentHour = Time.hour();
    Time.zone(4);
    int newHour = Time.hour();
    assertMoreOrEqual(4, (newHour-currentHour)%12);
    Time.zone(0);
}

test(TIME_Format) {

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
}

test(TIME_concatenate) {
    // addresses reports of timeStr() not being concatenatable
    time_t t = 1024*1024*1024;
    Time.zone(0);
    assertEqual(Time.timeStr(t).c_str(),(const char*)"Sat Jan 10 13:37:04 2004");
    String s = Time.timeStr(t);
    s += "abcd";
    assertEqual(s.c_str(), (const char*)"Sat Jan 10 13:37:04 2004abcd");
}

#endif