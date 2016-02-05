/**
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

#include "spark_wiring_ticks.h"
#include "system_setup.h"
#include "system_network.h"
#include "system_network_internal.h"
#include "system_cloud.h"
#include "system_event.h"
#include "system_threading.h"
#include "watchdog_hal.h"
#include "wlan_hal.h"
#include "delay_hal.h"
#include "rgbled.h"
#include <string.h>

uint32_t wlan_watchdog_base;
uint32_t wlan_watchdog_duration;

volatile uint8_t SPARK_WLAN_RESET;
volatile uint8_t SPARK_WLAN_SLEEP;
volatile uint8_t SPARK_WLAN_STARTED;


#if Wiring_WiFi
#include "system_network_wifi.h"
WiFiNetworkInterface wifi;
ManagedNetworkInterface& network = wifi;
inline NetworkInterface& nif(network_interface_t _nif) { return wifi; }
#define Wiring_Network 1
#endif

#if Wiring_Cellular
#include "system_network_cellular.h"
CellularNetworkInterface cellular;
ManagedNetworkInterface& network = cellular;
inline NetworkInterface& nif(network_interface_t _nif) { return cellular; }
#define Wiring_Network 1
#endif

#ifndef Wiring_Network
#define Wiring_Network 0
#endif

#if Wiring_Network

void HAL_WLAN_notify_simple_config_done()
{
    network.notify_listening_complete();
}

void HAL_NET_notify_connected()
{
    network.notify_connected();
}

void HAL_NET_notify_disconnected()
{
    network.notify_disconnected();
}

void HAL_NET_notify_can_shutdown()
{
    network.notify_can_shutdown();
}

void HAL_NET_notify_dhcp(bool dhcp)
{
    network.notify_dhcp(dhcp);
}


const void* network_config(network_handle_t network, uint32_t param, void* reserved)
{
    return nif(network).config();
}

void network_connect(network_handle_t network, uint32_t flags, uint32_t param, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).connect(!(flags & WIFI_CONNECT_SKIP_LISTEN)));
}

void network_disconnect(network_handle_t network, uint32_t param, void* reserved)
{
	nif(network).connect_cancel(true);
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).disconnect());
}

bool network_ready(network_handle_t network, uint32_t param, void* reserved)
{
    return nif(network).ready();
}

bool network_connecting(network_handle_t network, uint32_t param, void* reserved)
{
    return nif(network).connecting();
}

/**
 *
 * @param network
 * @param flags    1 - don't change the LED color
 * @param param
 * @param reserved
 */
void network_on(network_handle_t network, uint32_t flags, uint32_t param, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).on(!(flags & 1)));
}

bool network_has_credentials(network_handle_t network, uint32_t param, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(nif(network).has_credentials());
}

void network_off(network_handle_t network, uint32_t flags, uint32_t param, void* reserved)
{
    // flags & 1 means also disconnect the cloud (so it doesn't autmatically connect when network resumed.)
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).off(flags & 1));
}

/**
 *
 * @param network
 * @param flags  bit 0 set means to stop listening.
 * @param
 */
void network_listen(network_handle_t network, uint32_t flags, void*)
{
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).listen(flags & NETWORK_LISTEN_EXIT));
}

bool network_listening(network_handle_t network, uint32_t, void*)
{
    return nif(network).listening();
}

int network_set_credentials(network_handle_t network, uint32_t, NetworkCredentials* credentials, void*)
{
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(nif(network).set_credentials(credentials));
}

bool network_clear_credentials(network_handle_t network, uint32_t, NetworkCredentials* creds, void*)
{
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(nif(network).clear_credentials());
}

void network_setup(network_handle_t network, uint32_t flags, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).setup());
}


// These are internal methods
void manage_smart_config()
{
    network.listen_loop();
}

void manage_ip_config()
{
    nif(0).update_config();
}

#endif
