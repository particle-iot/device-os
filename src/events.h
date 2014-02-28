#ifndef __EVENTS_H
#define __EVENTS_H

#include <stdint.h>
#include <stdlib.h>

namespace EventType {
  enum Enum {
    PUBLIC = 'e',
    PRIVATE = 'E'
  };
}

size_t event(uint8_t buf[], uint16_t message_id, const char *event_name,
             const char *data, int ttl, EventType::Enum event_type);

size_t event_name_uri_path(uint8_t buf[], const char *name, size_t name_len);

#endif // __EVENTS_H
