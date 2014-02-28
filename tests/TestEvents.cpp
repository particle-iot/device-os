/**
  ******************************************************************************
  * @file    TestEvents.cpp
  * @authors Zachary Crockett
  * @version V1.0.0
  * @date    26-Feb-2014
  * @brief   Unit tests for CoAP event message creation
  ******************************************************************************
  Copyright (c) 2014 Spark Labs, Inc.  All rights reserved.

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
#include "UnitTest++.h"
#include "events.h"

static uint8_t buf[54];
size_t len;

SUITE(Events)
{
  TEST(LengthOfSingleCharEventWithNoDataIs8)
  {
    const size_t expected = 8;
    len = event(buf, 0xf649, "x", NULL, 60, EventType::PUBLIC);
    CHECK_EQUAL(expected, len);
  }

  TEST(ExpectedBufForSingleCharEventWithNoData)
  {
    const uint8_t expected[] = {
      0x50, 0x02, 0xf6, 0x49, 0xb1, 'e', 0x01, 'x' };
    len = event(buf, 0xf649, "x", NULL, 60, EventType::PUBLIC);
    CHECK_ARRAY_EQUAL(expected, buf, len);
  }

  TEST(LengthOfLongerMessageWithNoData)
  {
    const size_t expected = 27;
    len = event(buf, 0xf649, "front-door-unlocked", NULL, 60, EventType::PRIVATE);
    CHECK_EQUAL(expected, len);
  }

  TEST(ExpectedBufForLongerMessageWithNoData)
  {
    const uint8_t expected[] = {
      0x50, 0x02, 0xf6, 0x49, 0xb1, 'E', 0x0d, 0x06,
      'f','r','o','n','t','-','d','o','o','r','-',
      'u','n','l','o','c','k','e','d' };
    len = event(buf, 0xf649, "front-door-unlocked", NULL, 60, EventType::PRIVATE);
    CHECK_ARRAY_EQUAL(expected, buf, len);
  }

  TEST(LengthOfSingleCharEventNameUriPathIs2)
  {
    const size_t expected = 2;
    len = event_name_uri_path(buf, "x", 1);
    CHECK_EQUAL(expected, len);
  }

  TEST(ExpectedBufForSingleCharEventNameUriPath)
  {
    const uint8_t expected[] = { 0x01, 'x' };
    len = event_name_uri_path(buf, "x", 1);
    CHECK_ARRAY_EQUAL(expected, buf, len);
  }

  TEST(LengthOfLongerEventNameUriPath)
  {
    const char name[] = "front-door-unlocked";
    const size_t name_len = strlen(name);
    const size_t expected = name_len + 2;
    len = event_name_uri_path(buf, name, name_len);
    CHECK_EQUAL(expected, len);
  }

  TEST(ExpectedBufForLongerEventNameUriPath)
  {
    const uint8_t expected[] = { 0x0d, 0x06, 'f','r','o','n','t',
      '-','d','o','o','r','-','u','n','l','o','c','k','e','d' };
    len = event_name_uri_path(buf, "front-door-unlocked", 19);
    CHECK_ARRAY_EQUAL(expected, buf, len);
  }

  TEST(LengthOfLongEventWithDataAndTtl)
  {
    const size_t expected = 54;
    len = event(buf, 0x7654, "weather/us/mn/minneapolis",
                "t:5F,d:-2F,p:15%", 3600, EventType::PUBLIC);
    CHECK_EQUAL(expected, len);
  }

  TEST(ExpectedBufForLongEventWithDataAndTtl)
  {
    const uint8_t expected[] = {
      0x50, 0x02, 0x76, 0x54, 0xb1, 'e', 0x0d, 0x0c,
      'w','e','a','t','h','e','r','/','u','s','/','m','n','/',
      'm','i','n','n','e','a','p','o','l','i','s',
      0x33, 0x00, 0x0e, 0x10, 0xff,
      't',':','5','F',',','d',':','-','2','F',',','p',':','1','5','%' };
    len = event(buf, 0x7654, "weather/us/mn/minneapolis",
                "t:5F,d:-2F,p:15%", 3600, EventType::PUBLIC);
    CHECK_ARRAY_EQUAL(expected, buf, len);
  }
}
