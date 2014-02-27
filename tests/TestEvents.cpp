#include "UnitTest++.h"
#include "events.h"

SUITE(Events)
{
  TEST(LengthOfSingleCharEventWithNoDataIs8)
  {
    uint8_t buf[8];
    size_t len = event(buf, 0xf649, "x", NULL, 60, EventType::PUBLIC);
    CHECK_EQUAL(8, len);
  }

  TEST(ExpectedBufForSingleCharEventWithNoData)
  {
    const uint8_t expected[8] = {
      0x50, 0x02, 0xf6, 0x49, 0xb1, 'e', 0x01, 'x' };
    uint8_t buf[8];
    size_t len = event(buf, 0xf649, "x", NULL, 60, EventType::PUBLIC);
    CHECK_ARRAY_EQUAL(expected, buf, len);
  }

  TEST(LengthOfLongerMessageWithNoData)
  {
    uint8_t buf[27];
    size_t len = event(buf, 0xf649, "front-door-unlocked", NULL, 60, EventType::PRIVATE);
    CHECK_EQUAL(27, len);
  }

  TEST(ExpectedBufForLongerMessageWithNoData)
  {
    const uint8_t expected[27] = {
      0x50, 0x02, 0xf6, 0x49, 0xb1, 'E', 0x0d, 0x06,
      'f','r','o','n','t','-','d','o','o','r','-',
      'u','n','l','o','c','k','e','d' };
    uint8_t buf[27];
    size_t len = event(buf, 0xf649, "front-door-unlocked", NULL, 60, EventType::PRIVATE);
    CHECK_ARRAY_EQUAL(expected, buf, len);
  }

  TEST(LengthOfSingleCharEventNameUriPathIs2)
  {
    uint8_t buf[2];
    size_t len = event_name_uri_path(buf, "x", 1);
    CHECK_EQUAL(2, len);
  }

  TEST(LengthOfLongerEventNameUriPath)
  {
    const char name[] = "front-door-unlocked";
    const char name_len = strlen(name);
    uint8_t buf[21];
    size_t len = event_name_uri_path(buf, name, name_len);
    CHECK_EQUAL(21, len);
  }
}
