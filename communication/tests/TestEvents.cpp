/**
  ******************************************************************************
  * @file    TestEvents.cpp
  * @authors Zachary Crockett
  * @version V1.0.0
  * @date    26-Feb-2014
  * @brief   Unit tests for CoAP event message creation
  ******************************************************************************
  Copyright (c) 2014-2015 Particle Industries, Inc.  All rights reserved.

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
#include <string.h>
#include "UnitTest++.h"
#include "events.h"

static uint8_t buf[54];
size_t len;

SUITE(Events)
{
  /***** publishing *****/

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


  /***** subscribing *****/

  TEST(LengthOfSubscriptionToOneDeviceFiltered)
  {
    const size_t expected = 39;
    len = subscription(buf, 0x8888, "weather", "53ff73065067544816300187");
    CHECK_EQUAL(expected, len);
  }

  TEST(ExpectedBufForSubscriptionToOneDeviceFiltered)
  {
    const uint8_t expected[] = {
      0x40, 0x01, 0x88, 0x88, 0xB1, 'e', 0x07,
      'w', 'e', 'a', 't', 'h', 'e', 'r', 0xFF,
      '5','3','f','f','7','3','0','6','5','0','6','7',
      '5','4','4','8','1','6','3','0','0','1','8','7' };
    len = subscription(buf, 0x8888, "weather", "53ff73065067544816300187");
    CHECK_ARRAY_EQUAL(expected, buf, len);
  }

  TEST(LengthOfSubscriptionToOneDeviceUnfilteredWithNull)
  {
    const size_t expected = 31;
    len = subscription(buf, 0x7000, NULL, "53ff73065067544816300187");
    CHECK_EQUAL(expected, len);
  }

  TEST(LengthOfSubscriptionToOneDeviceUnfilteredWithEmptyString)
  {
    const size_t expected = 31;
    len = subscription(buf, 0x7000, "", "53ff73065067544816300187");
    CHECK_EQUAL(expected, len);
  }

  TEST(ExpectedBufForSubscriptionToOneDeviceUnfilteredWithNull)
  {
    const uint8_t expected[] = {
      0x40, 0x01, 0x70, 0x00, 0xB1, 'e', 0xFF,
      '5','3','f','f','7','3','0','6','5','0','6','7',
      '5','4','4','8','1','6','3','0','0','1','8','7' };
    len = subscription(buf, 0x7000, NULL, "53ff73065067544816300187");
    CHECK_ARRAY_EQUAL(expected, buf, len);
  }

  TEST(ExpectedBufForSubscriptionToOneDeviceUnfilteredWithEmptyString)
  {
    const uint8_t expected[] = {
      0x40, 0x01, 0x70, 0x00, 0xB1, 'e', 0xFF,
      '5','3','f','f','7','3','0','6','5','0','6','7',
      '5','4','4','8','1','6','3','0','0','1','8','7' };
    len = subscription(buf, 0x7000, "", "53ff73065067544816300187");
    CHECK_ARRAY_EQUAL(expected, buf, len);
  }

  TEST(LengthOfSubscriptionToMyDevicesFiltered)
  {
    const size_t expected = 20;
    len = subscription(buf, 0x1113, "motion/open", SubscriptionScope::MY_DEVICES);
    CHECK_EQUAL(expected, len);
  }

  TEST(ExpectedBufForSubscriptionToMyDevicesFiltered)
  {
    const uint8_t expected[] = {
      0x40, 0x01, 0x11, 0x13, 0xB1, 'e', 0x0B,
      'm', 'o', 't', 'i', 'o', 'n', '/', 'o', 'p', 'e', 'n',
      0x41, 'u' };
    len = subscription(buf, 0x1113, "motion/open", SubscriptionScope::MY_DEVICES);
    CHECK_ARRAY_EQUAL(expected, buf, len);
  }

  TEST(LengthOfSubscriptionToMyDevicesUnfiltered)
  {
    const size_t expected = 8;
    len = subscription(buf, 0x1114, NULL, SubscriptionScope::MY_DEVICES);
    CHECK_EQUAL(expected, len);
  }

  TEST(ExpectedBufForSubscriptionToMyDevicesUnfiltered)
  {
    const uint8_t expected[] = {
      0x40, 0x01, 0x11, 0x14, 0xB1, 'e', 0x41, 'u' };
    len = subscription(buf, 0x1114, NULL, SubscriptionScope::MY_DEVICES);
    CHECK_ARRAY_EQUAL(expected, buf, len);
  }

  TEST(LengthOfSubscriptionToFirehoseFiltered)
  {
    const size_t expected = 34;
    len = subscription(buf, 0x1115, "China/ShenZhen/FuTianKouAn",
                       SubscriptionScope::FIREHOSE);
    CHECK_EQUAL(expected, len);
  }

  TEST(ExpectedBufForSubscriptionToFirehoseFiltered)
  {
    const uint8_t expected[] = {
      0x40, 0x01, 0x11, 0x15, 0xB1, 'e', 0x0D, 0x0D,
      'C','h','i','n','a','/','S','h','e','n','Z','h','e','n','/',
      'F','u','T','i','a','n','K','o','u','A','n' };
    len = subscription(buf, 0x1115, "China/ShenZhen/FuTianKouAn",
                       SubscriptionScope::FIREHOSE);
    CHECK_ARRAY_EQUAL(expected, buf, len);
  }

  TEST(LengthOfDisallowedFirehoseUnfiltered)
  {
    const size_t expected = -1;
    len = subscription(buf, 0x1116, NULL, SubscriptionScope::FIREHOSE);
    CHECK_EQUAL(expected, len);
  }
}
