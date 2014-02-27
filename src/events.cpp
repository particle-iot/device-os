#include "events.h"
#include <string.h>

size_t event(uint8_t buf[], uint16_t message_id, const char *event_name,
             const char *data, int ttl, EventType::Enum event_type)
{
  size_t name_len = strnlen(event_name, 63);
  int message_len = name_len + 7 + (name_len < 13 ? 0 : 1);

  buf[0] = 0x50; // non-confirmable, no token
  buf[1] = 0x02; // code 0.02 POST request
  buf[2] = message_id >> 8;
  buf[3] = message_id & 0xff;
  buf[4] = 0xb1;
  buf[5] = event_type;

  size_t option_len = event_name_uri_path(buf + 6, event_name, name_len);

  return message_len;
}

size_t event_name_uri_path(uint8_t buf[], const char *name, size_t name_len)
{
  if (name_len < 13) {
    buf[0] = name_len;
    memcpy(buf + 1, name, name_len);
    return name_len + 1;
  } else {
    buf[0] = 0x0d;
    buf[1] = name_len - 13;
    memcpy(buf + 2, name, name_len);
    return name_len + 2;
  }
}
