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

size_t event(uint8_t[], uint16_t, const char *, const char *, int, EventType::Enum);
size_t event_name_uri_path(uint8_t[], const char *, size_t);

#endif // __EVENTS_H
