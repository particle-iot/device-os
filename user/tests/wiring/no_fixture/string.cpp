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
#include "spark_wiring_string.h"


#if PLATFORM_ID>=3

test(STRING_01_float_conversion) {
    String one(1);
    assertEqual(String(1.000000, 0), one);
}

test(STRING_02_float_negative) {
    String s(-123.456, 3);
    assertTrue(!strcmp("-123.456", s));
}

test(STRING_03_float_no_decimals_rounding_up) {
    String s(123.9, 0);
    assertTrue(!strcmp("124", s));
}

test(STRING_04_float_no_decimals_round_down) {
    String s(123.2, 0);
    assertTrue(!strcmp("123", s));
}

test(STRING_05_float_negative_no_decimals_rounding_up) {
    String s(-123.9, 0);
    assertTrue(!strcmp("-124", s));
}

test(STRING_06_float_negative_no_decimals_round_down) {
    String s(-123.2, 0);
    assertTrue(!strcmp("-123", s));
}

test(STRING_07_bug_563_integer_overflow) {
    String s(-5000.0);
    assertTrue(!strcmp("-5000.000000", s));
}

test(STRING_08_sprintf) {
    char buf[20];
    sprintf(buf, "%f", 1.0f);
    assertMore(strlen(buf), 0);
    assertEqual((const char*)buf, (const char*)"1.000000");
}

test(STRING_09_remove_to_end) {

    assertEqual((const char*)"123", String("123abcd").remove(3));
    assertEqual((const char*)"123456", String("123ab456").remove(3,2));
}

test(STRING_10_float) {

    assertEqual(0.0f, String("abcd").toFloat());
    assertEqual(0.0f, String("0.0").toFloat());
    assertEqual(123.00f, String("123").toFloat());
    assertEqual(123.456f, String("123.456").toFloat());
    assertEqual(123.00f, String("123abcd").toFloat());
}

#endif

test(STRING_11_printable_constructor)
{

    IPAddress address(1,2,3,4);
    assertEqual((const char*)"1.2.3.4", String(address));
}