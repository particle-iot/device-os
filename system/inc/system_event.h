/*
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

#pragma once

#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif


typedef uint64_t system_event_t;
typedef void (system_event_handler_t)(system_event_t event, uint32_t param, void* pointer);


enum SystemEvents {

    // begin/progress/end
    // wifi, cloud, ota_update
    // custom flags

    wifi_listen_begin = 1<<1,
    wifi_listen_update = 1<<2,
    wifi_listen_end = 1<<3,
    wifi_listen = wifi_listen_begin + wifi_listen_update + wifi_listen_end,
    wifi_credentials_cleared = 1<<4,
    wifi_connecting = 1<<5,
    wifi_connect = 1<<6,
    wifi_disconnect = 1<<7,
    firmware_update = 1<<8,          // parameter is 0 for begin, 1 for OTA complete, -1 for error.


    all_events = 0x7FFFFFFF
};

enum SystemEventsParam {
    wifi_credentials_add = 1,
    wifi_credentials_clear = 2,
    firmware_update_failed = -1,
    firmware_update_begin = 0,
    firmware_update_complete = 1,
    firmware_update_progress = 2
};


/**
 * Subscribes to the system events given
 * @param events    One or more system events. Multiple system events are specified using the + operator.
 * @param handler   The system handler function to call.
 * @param reserved  Set to NULL.
 * @return {@code 0} if the system event handlers were registered succesfully. Non-zero otherwise.
 */
int system_subscribe_event(system_event_t events, system_event_handler_t* handler, void* reserved);

/**
 * Unsubscribes a handler from the given events.
 * @param handler   The handler that will be unsubscribed.
 * @param reserved  Set to NULL.
 */
void system_unsubscribe_event(system_event_t events, system_event_handler_t* handler, void* reserved);

#ifdef __cplusplus
}
#endif


/**
 * Notifes all subscribers about an event.
 * @param event
 * @param data
 * @param pointer
 */
void system_notify_event(system_event_t event, uint32_t data=0, void* pointer=nullptr);
