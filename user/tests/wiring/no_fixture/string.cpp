/**
 ******************************************************************************
 * @file    string.cpp
 * @authors mat
 * @date    06 April 2015
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

#include "application.h"
#include "unit-test/unit-test.h"
#include "particle_wiring_string.h"

test(String_float_conversion) {
    String one(1);
    assertEqual(String(1.000000, 0), one);
}

test(String_float_negative) {
    String s(-123.456, 3);
    assertTrue(!strcmp("-123.456", s));
}

test(String_float_no_decimals_rounding_up) {
    String s(123.9, 0);
    assertTrue(!strcmp("124", s));
}

test(String_float_no_decimals_round_down) {
    String s(123.2, 0);
    assertTrue(!strcmp("123", s));
}

test(String_float_negative_no_decimals_rounding_up) {
    String s(-123.9, 0);
    assertTrue(!strcmp("-124", s));
}

test(String_float_negative_no_decimals_round_down) {
    String s(-123.2, 0);
    assertTrue(!strcmp("-123", s));
}

test(String_bug_563_integer_overflow) {
    String s(-5000.0);
    assertTrue(!strcmp("-5000.000000", s));
}

test(String_sprintf) {
    char buf[20];
    sprintf(buf, "%f", 1.0f);
    assertMore(strlen(buf), 0);
    assertEqual((const char*)buf, (const char*)"1.000000");
}
