/**
 ******************************************************************************
 * @file    system_task.cpp
 * @authors mat
 * @date    24 February 2015
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#include "system_task.h"
#include "system_version.h"
#include "spark_macros.h"
#include "spark_wiring_system.h"
#undef WARN
#undef INFO

#include "catch.hpp"

#include <algorithm>

using std::min;

SCENARIO("Backoff period after 0 attempts should be 0", "[system_task]") {

    REQUIRE(backoff_period(0)==0);
}

SCENARIO("Backoff period should increase exponentially from 1s to 128s", "[system_task]") {

    unsigned previous = 0;
    for (int i=0; i<1000; i++) {
        INFO("connection attempts " << i);
        unsigned period = backoff_period(i);
        REQUIRE(period >= previous);
        int exponent = min(7,  ((i-1)/5));
        unsigned expected = i==0 ? 0 : ((1<<exponent)*1000);
        REQUIRE(period == expected);
        previous = period;
    }
}

SCENARIO("System version info is retrieved", "[system,version]") {

    SystemVersionInfo info;

    int size = system_version_info(&info, nullptr);
    REQUIRE(size==sizeof(info));

    REQUIRE(info.versionNumber==SYSTEM_VERSION);


    char expected[20];
    // Order of testing here is important to retain
    if ((SYSTEM_VERSION & 0xFF) == 0xFF) {
        sprintf(expected, "%d.%d.%d", BYTE_N(info.versionNumber,3), BYTE_N(info.versionNumber,2), BYTE_N(info.versionNumber,1));
    } else if ((SYSTEM_VERSION & 0xC0) == 0x00) {
        sprintf(expected, "%d.%d.%d-alpha.%d", BYTE_N(info.versionNumber,3), BYTE_N(info.versionNumber,2), BYTE_N(info.versionNumber,1), BYTE_N(info.versionNumber,0) & 0x3F);
    } else if ((SYSTEM_VERSION & 0xC0) == 0x40) {
        sprintf(expected, "%d.%d.%d-beta.%d", BYTE_N(info.versionNumber,3), BYTE_N(info.versionNumber,2), BYTE_N(info.versionNumber,1), BYTE_N(info.versionNumber,0) & 0x3F);
    } else if ((SYSTEM_VERSION & 0xC0) == 0x80) {
        sprintf(expected, "%d.%d.%d-rc.%d", BYTE_N(info.versionNumber,3), BYTE_N(info.versionNumber,2), BYTE_N(info.versionNumber,1), BYTE_N(info.versionNumber,0) & 0x3F);
    } else if ((SYSTEM_VERSION & 0xC0) >= 0xC0) {
        FAIL("expected \"alpha\", \"beta\", \"rc\", or \"default\" version!");
    }

    REQUIRE_THAT( expected, Equals(info.versionString));

    REQUIRE(System.versionNumber()==info.versionNumber);

    REQUIRE(System.version()==info.versionString);
}

// these symbols needed for successful link

volatile uint8_t SPARK_CLOUD_CONNECT;
volatile uint8_t SPARK_WLAN_SLEEP;

SystemClass System;
