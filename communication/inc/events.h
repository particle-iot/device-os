/**
  ******************************************************************************
  * @file    events.h
  * @authors Zachary Crockett
  * @version V1.0.0
  * @date    26-Feb-2014
  * @brief   Internal CoAP event message creation
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
#ifndef __EVENTS_H
#define __EVENTS_H

#include <stdint.h>
#include <stdlib.h>
#include "platforms.h"
#include "protocol_defs.h"

namespace EventType {
  enum Enum : char {
    PUBLIC = 'e', // Deprecated (0x65)
    PRIVATE = 'E', // Deprecated (0x45)
  };

  /**
   * These flags are encoded into the same 32-bit integer that already holds EventType::Enum
   */
  enum Flags {
    EMPTY_FLAGS = 0,
    NO_ACK = 0x2,
    WITH_ACK = 0x8,
    ASYNC = 0x10,        // not used here, but reserved since it's used in the system layer. Makes conversion simpler.
    ALL_FLAGS = NO_ACK | WITH_ACK | ASYNC
  };

  static_assert((PUBLIC & NO_ACK)==0 &&
	  (PRIVATE & NO_ACK)==0 &&
	  (PUBLIC & WITH_ACK)==0 &&
	  (PRIVATE & WITH_ACK)==0 &&
	  (PRIVATE & ASYNC)==0 &&
	  (PUBLIC & ASYNC)==0, "flags should be distinct from event type");
} // namespace EventType

#if PLATFORM_ID != PLATFORM_GCC
static_assert(sizeof(EventType::Enum) == 1, "EventType::Enum size is not 1");
#endif

namespace SubscriptionFlag {
  enum Enum {
    MY_DEVICES = 0x00, // Deprecated
    FIREHOSE = 0x01, // Deprecated
    BINARY_DATA = 0x02, // The subscription handler accepts binary data
    CBOR_DATA = 0x04, // The subscription handler accepts CBOR data
    LARGE_EVENT = 0x08 // The subscription handler supports large events
  };
}

#if PLATFORM_ID != PLATFORM_GCC
static_assert(sizeof(SubscriptionFlag::Enum) == 1, "SubscriptionFlag::Enum size is not 1");
#endif

typedef void (*EventHandler)(const char *event_name, const char *data);
typedef void (*EventHandlerWithData)(void *handler_data, const char *event_name, const char *data, size_t data_size,
    int content_type);

/**
 *  This is used in a callback so only change by adding fields to the end
 */
struct FilteringEventHandler
{
  char filter[64]; // XXX: Not null-terminated if 64 characters long
  EventHandler handler;
  void *handler_data;
  uint8_t flags;
  char device_id[13]; // XXX: Unused field. Keeping for ABI compatibility for now
};

#endif // __EVENTS_H
