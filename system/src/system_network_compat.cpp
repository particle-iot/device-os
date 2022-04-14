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

#include "hal_platform.h"
/* FIXME: there should be a define that tells whether there is NetworkManager available
 * or not */
#if !HAL_PLATFORM_IFAPI

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

uint32_t wlan_watchdog_base = 0;
uint32_t wlan_watchdog_duration = 0;

volatile uint8_t SPARK_WLAN_RESET = 0;
volatile uint8_t SPARK_WLAN_SLEEP = 0;
volatile uint8_t SPARK_WLAN_CONNECT_RESTORE = 0;
volatile uint8_t SPARK_WLAN_STARTED = 0;


#if Wiring_WiFi
#include "system_network_wifi.h"
#include "spark_wiring_wifi.h"
WiFiNetworkInterface wifi;
ManagedNetworkInterface& network = wifi;
inline NetworkInterface& nif(network_interface_t _nif) { return wifi; }
#define Wiring_Network 1
#endif

#if Wiring_Cellular
#include "system_network_cellular.h"
#include "spark_wiring_cellular.h"
CellularNetworkInterface cellular;
ManagedNetworkInterface& network = cellular;
inline NetworkInterface& nif(network_interface_t _nif) { return cellular; }
#define Wiring_Network 1
#endif

#ifndef Wiring_Network
#define Wiring_Network 0
#else

void HAL_WLAN_notify_simple_config_done()
{
    invokeAsync([]() {
        network.notify_listening_complete();
    });
}

void HAL_NET_notify_connected()
{
    invokeAsync([]() {
        network.notify_connected();
    });
}

void HAL_NET_notify_disconnected()
{
    invokeAsync([]() {
        network.notify_disconnected();
    });
}

void HAL_NET_notify_error()
{
    invokeAsync([]() {
        network.notify_error();
    });
}

void HAL_NET_notify_dhcp(bool dhcp)
{
    invokeAsync([dhcp]() {
        network.notify_dhcp(dhcp);
    });
}

const void* network_config(network_handle_t network, uint32_t param, void* reserved)
{
    nif(network).update_config(true);
    return nif(network).config();
}

void network_connect(network_handle_t network, uint32_t flags, uint32_t param, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).connect(!(flags & WIFI_CONNECT_SKIP_LISTEN)));
}

void network_disconnect(network_handle_t network, uint32_t reason, void* reserved)
{
	nif(network).connect_cancel(true);
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).disconnect((network_disconnect_reason)reason));
}

bool network_ready(network_handle_t network, uint32_t param, void* reserved)
{
    return nif(network).ready();
}

bool network_connecting(network_handle_t network, uint32_t param, void* reserved)
{
    return nif(network).connecting();
}

int network_connect_cancel(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved) {
    nif(network).connect_cancel(flags);
    return 0;
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
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).on());
}

bool network_is_on(network_handle_t network, void* reserved) {
    return nif(network).isOn();
}

bool network_is_off(network_handle_t network, void* reserved) {
    return nif(network).isOff();
}

bool network_has_credentials(network_handle_t network, uint32_t param, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(nif(network).has_credentials());
}

void network_off(network_handle_t network, uint32_t flags, uint32_t param, void* reserved)
{
    nif(network).connect_cancel(true);
    // flags & 1 means also disconnect the cloud (so it doesn't autmatically connect when network resumed.)
    SYSTEM_THREAD_CONTEXT_ASYNC_CALL(nif(network).off(flags & 1));
}

int network_wait_off(network_handle_t network, system_tick_t timeout, void*) {
    // Temporarily not necessary for Gen2 platforms.
    return SYSTEM_ERROR_NONE;
}

/**
 *
 * @param network
 * @param flags  bit 0 set means to stop listening.
 * @param
 */
void network_listen(network_handle_t network, uint32_t flags, void*)
{
    const bool stop = flags & NETWORK_LISTEN_EXIT;
    // Set/clear listening mode flag
    nif(network).listen(stop);
    if (!stop) {
        // Cancel current connection attempt
        nif(network).connect_cancel(true);
    }
}

int network_listen_sync(network_handle_t network, uint32_t flags, void*) {
    network_listen(network, flags, nullptr);
    return 0;
}

void network_set_listen_timeout(network_handle_t network, uint16_t timeout, void*)
{
    return nif(network).set_listen_timeout(timeout);
}

uint16_t network_get_listen_timeout(network_handle_t network, uint32_t flags, void*)
{
    return nif(network).get_listen_timeout();
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

int network_set_hostname(network_handle_t network, uint32_t flags, const char* hostname, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(nif(network).set_hostname(hostname));
}

int network_get_hostname(network_handle_t network, uint32_t flags, char* buffer, size_t buffer_len, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_SYNC_CALL_RESULT(nif(network).get_hostname(buffer, buffer_len));
}

int network_listen_command(network_handle_t network, network_listen_command_t command, void* arg)
{
    nif(network).listen_command();
    return 0;
}

/* FIXME: */
extern int cfod_count;

/**
 * Reset or initialize the network connection as required.
 */
void manage_network_connection()
{
    if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || WLAN_WD_TO())
    {
        if (SPARK_WLAN_STARTED)
        {
            WARN("Resetting WLAN due to %s", (WLAN_WD_TO()) ? "WLAN_WD_TO()":((SPARK_WLAN_RESET) ? "SPARK_WLAN_RESET" : "SPARK_WLAN_SLEEP"));
            auto was_sleeping = SPARK_WLAN_SLEEP;
            SPARK_WLAN_CONNECT_RESTORE = network_ready(0, 0, 0) || network_connecting(0, 0, 0);
            //auto was_disconnected = network.manual_disconnect();
            // Note: The cloud connectivity layer may "detect" an unanticipated network disconnection
            // before the networking layer, and due to current recovery logic, which resets the network,
            // it's difficult to say whether the network has actually failed or not. In this case we
            // disconnect from the network with the RESET reason code
            network_disconnect(0, SPARK_WLAN_SLEEP ? NETWORK_DISCONNECT_REASON_UNKNOWN : NETWORK_DISCONNECT_REASON_RESET, 0);
            network_off(0, 0, 0, 0);
            CLR_WLAN_WD();
            SPARK_WLAN_RESET = 0;
            SPARK_WLAN_SLEEP = was_sleeping;
            //network.set_manual_disconnect(was_disconnected);
            cfod_count = 0;
        }
    }
    else
    {
        if (!SPARK_WLAN_STARTED) {
            // INFO("Network On: !SPARK_WLAN_STARTED");
            network_on(0, 0, 0, 0);
        }

        if ((SPARK_WLAN_CONNECT_RESTORE || spark_cloud_flag_auto_connect()) && !network_ready(0, 0, 0)) {
            // INFO("Network Connect:%s%s", (SPARK_WLAN_CONNECT_RESTORE ? " SPARK_WLAN_CONNECT_RESTORE" : " "),
            //                              (spark_cloud_flag_auto_connect() ? " spark_cloud_flag_auto_connect()" : " "));

            // XXX: If the auto-connect flag is not set, we used to only call network_connect() once here,
            // even if it failed to connect to network for whatever reason. So we should clear the flag here.
            SPARK_WLAN_CONNECT_RESTORE = 0;
            network_connect(0, 0, 0, 0);
        } else {
            nif(0).process();
        }
    }
}

#endif /* Wiring_Network */
#endif /* !HAL_PLATFORM_IFAPI */
