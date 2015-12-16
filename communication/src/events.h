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

namespace EventType {
  enum Enum {
    PUBLIC = 'e',
    PRIVATE = 'E'
  };
}

namespace SubscriptionScope {
  enum Enum {
    MY_DEVICES,
    FIREHOSE
  };
}

typedef void (*EventHandler)(const char *event_name, const char *data);
typedef void (*EventHandlerWithData)(void *handler_data, const char *event_name, const char *data);

/**
 *  This is used in a callback so only change by adding fields to the end
 */
struct FilteringEventHandler
{
  char filter[64];
  EventHandler handler;
  void *handler_data;
  SubscriptionScope::Enum scope;
  char device_id[13];
};


size_t subscription(uint8_t buf[], uint16_t message_id,
                    const char *event_name, const char *device_id);

size_t subscription(uint8_t buf[], uint16_t message_id,
                    const char *event_name, SubscriptionScope::Enum scope);

size_t event_name_uri_path(uint8_t buf[], const char *name, size_t name_len);

#endif // __EVENTS_H
