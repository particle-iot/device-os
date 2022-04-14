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
typedef void (system_event_handler_t)(system_event_t event, int param, void* pointer, void* context);


enum SystemEvents {

    // begin/progress/end
    // wifi, cloud, ota_update
    // custom flags

    wifi_listen_begin = 1<<1,
    wifi_listen_update = 1<<2,
    wifi_listen_end = 1<<3,
    wifi_listen = wifi_listen_begin + wifi_listen_update + wifi_listen_end,
    setup_begin = wifi_listen_begin,
    setup_update = wifi_listen_update,
    setup_end = wifi_listen_end,
    setup_all = wifi_listen,
    network_credentials = 1<<4,
    network_status = 1<<5,
    cloud_status = 1<<6,             // parameter is 0 for disconnected, 1 for connecting, 8 for connected, 9 for disconnecting. other values reserved.
    button_status = 1<<7,            // parameter is >0 for time pressed in ms (when released) or 0 for just pressed.
    firmware_update = 1<<8,          // parameter is 0 for begin, 1 for OTA complete, -1 for error.
    firmware_update_pending = 1<<9,	// notifies the application that an OTA update is pending and will be delivered when updates are enabled
    reset_pending = 1<<10,          // notifies that the system would like to shutdown (System.resetPending() return true)
    // todo - rename to system_reset, or otherwise avoid common name clashes
	reset = 1<<11,                  // notifies that the system will now reset on return from this event.
    button_click = 1<<12,           // generated for every click in series - data is number of clicks in the lower 4 bits.
    button_final_click = 1<<13,     // generated for last click in series - data is the number of clicks in the lower 4 bits.
    time_changed = 1<<14,
    low_battery = 1<<15,            // generated when low battery condition is detected
    battery_state = 1<<16,
    power_source = 1<<17,
    out_of_memory = 1<<18,          // heap request was not satisfied
    ble_prov_mode = 1<<19,          // ble provisioning mode

    all_events = 0xFFFFFFFFFFFFFFFF
};

inline uint8_t system_button_clicks(int param) { return param&0xF; }

enum SystemEventsParam {
    //
    network_credentials_added = 1,
    network_credentials_cleared = 2,
    firmware_update_failed = (uint32_t)-1,
    firmware_update_begin = 0,
    firmware_update_complete = 1,
    firmware_update_progress = 2,

    // Network status. bit 0 is clear if it's a transition state (e.g. powering up, connecting), is set when it's a rest state (connected/ready etc)
    network_status_powering_off     = 1<<1 | 0,
    network_status_off              = 1<<1 | 1,
    network_status_powering_on      = 2<<1 | 0,
    network_status_on               = 2<<1 | 1,
    network_status_connecting       = 3<<1 | 0,
    network_status_connected        = 3<<1 | 1,
    // network_status_preparing        = 4<<1 | 0,
    // network_status_ready            = 4<<1 | 1,
    network_status_disconnecting    = 5<<1 | 0,
    network_status_disconnected     = 5<<1 | 1,

    // Cloud connection status
    cloud_status_disconnected       = 0,
    cloud_status_connecting         = 1,
    cloud_status_handshake          = 2,
    cloud_status_session_resume     = 3,
    cloud_status_connected          = 8,
    cloud_status_disconnecting      = 9,

    time_changed_manually = 0,
    time_changed_sync = 1,

    // BLE provisioning mode
    ble_prov_mode_connected = 0,
    ble_prov_mode_disconnected = 1,
    ble_prov_mode_handshake_failed = 2,
    ble_prov_mode_handshake_done = 3

};

/**
 * Flags altering the behavior of the `system_notify_event()` function.
 */
enum SystemNotifyEventFlag {
    NOTIFY_SYNCHRONOUSLY = 0x01
};

#define SYSTEM_EVENT_CONTEXT_VERSION        (2)

typedef struct SystemEventContext {
    uint16_t version;
    uint16_t size;
    void* callable;
    void (*destructor)(void* callable);
} SystemEventContext;

/**
 * Subscribes to the system events given
 * @param events    One or more system events. Multiple system events are specified using the + operator.
 * @param handler   The system handler function to call.
 * @param context   Context along with the handler function.
 * @return {@code 0} if the system event handlers were registered successfully. Non-zero otherwise.
 */
int system_subscribe_event(system_event_t events, system_event_handler_t* handler, SystemEventContext* context);

/**
 * Unsubscribes a handler from the given events.
 * @param handler   The handler that will be unsubscribed.
 * @param context   Event subscription context.
 */
void system_unsubscribe_event(system_event_t events, system_event_handler_t* handler, const SystemEventContext* context);

void system_notify_time_changed(uint32_t data, void* reserved, void* reserved1);

#ifdef __cplusplus
}
#endif


/**
 * Notifies all subscribers about an event. It is safe to call this function from an ISR.
 * @param event
 * @param data
 * @param pointer
 * @param flags Event flags as defined by the `SystemNotifyEventFlag` enum.
 */
void system_notify_event(system_event_t event, uint32_t data = 0, void* pointer = nullptr,
        void (*fn)(void* data) = nullptr, void* fndata = nullptr, unsigned flags = 0);
