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

