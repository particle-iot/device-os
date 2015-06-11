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

#if 0

#include "system_task.h"

unsigned backoff_period(uint8_t connection_attempts);

SCENARIO("Backoff period after 0 attempts should be 0", "[system_task]") {
    
    REQUIRE(backoff_period(0)==0);
    
}

SCENARIO("Backoff period should increase exponentially to a point where the backoff period is at least 5 minutes, and no more than 10 minutes.", "[system_task]") {
    
    unsigned sum = 0;
    unsigned previous = 0;
    for (int i=1; i<13; i++) {
        unsigned period = backoff_period(i);
        REQUIRE(period > previous);
        REQUIRE(period > sum);
        sum += period;
        previous = period;
    }    
}


SCENARIO("Backoff period has a maximum of 5 minutes, and no more than 10 minutes.", "[system_task]") {
        
    unsigned previous = 0;
    for (int i=1; i<200; i++) {
        unsigned period = backoff_period(i);
        REQUIRE(period >= prevoius);
        previous = period;        
    }
    
    REQUIRE(previous > 5*60*1000);
    REQUIRE(previous < 10*60*1000);
}

#endif
