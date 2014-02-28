#include "events.h"
#include <string.h>

size_t event(uint8_t buf[], uint16_t message_id, const char *event_name,
             const char *data, int ttl, EventType::Enum event_type)
{
  uint8_t *p = buf;
  *p++ = 0x50; // non-confirmable, no token
  *p++ = 0x02; // code 0.02 POST request
  *p++ = message_id >> 8;
  *p++ = message_id & 0xff;
  *p++ = 0xb1;
  *p++ = event_type;

  size_t name_data_len = strnlen(event_name, 63);
  p += event_name_uri_path(p, event_name, name_data_len);

  if (60 != ttl)
  {
    *p++ = 0x33;
    *p++ = (ttl >> 16) & 0xff;
    *p++ = (ttl >> 8) & 0xff;
    *p++ = ttl & 0xff;
  }

  if (NULL != data)
  {
    name_data_len = strnlen(data, 63);

    *p++ = 0xff;
    memcpy(p, data, name_data_len);
    p += name_data_len;
  }

  return p - buf;
}

size_t event_name_uri_path(uint8_t buf[], const char *name, size_t name_len)
{
  if (name_len < 13)
  {
    buf[0] = name_len;
    memcpy(buf + 1, name, name_len);
    return name_len + 1;
  }
  else
  {
    buf[0] = 0x0d;
    buf[1] = name_len - 13;
    memcpy(buf + 2, name, name_len);
    return name_len + 2;
  }
}
