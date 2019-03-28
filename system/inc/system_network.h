/**
 ******************************************************************************
 * @file    system_network.h
 * @authors Matthew McGowan
 * @date    12 February 2015
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

#ifndef SYSTEM_NETWORK_H
#define	SYSTEM_NETWORK_H

#include "inet_hal.h"
#include "wlan_hal.h"
#include "spark_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PARTICLE_NO_NETWORK
#define PARTICLE_NO_NETWORK 0
#endif

#if PARTICLE_NO_NETWORK
#undef SPARK_NO_CLOUD
#define SPARK_NO_CLOUD 1
#endif

typedef enum network_interface_index {
    NETWORK_INTERFACE_ALL = 0,
    NETWORK_INTERFACE_LOOPBACK = 1,
    NETWORK_INTERFACE_MESH = 2,
    NETWORK_INTERFACE_ETHERNET = 3,
    NETWORK_INTERFACE_CELLULAR = 4,
    NETWORK_INTERFACE_WIFI_STA = 4,
    NETWORK_INTERFACE_WIFI_AP = 5
} network_interface_index;

typedef enum network_disconnect_reason {
    NETWORK_DISCONNECT_REASON_NONE = 0,
    NETWORK_DISCONNECT_REASON_ERROR = 1, // Disconnected due to an error
    NETWORK_DISCONNECT_REASON_USER = 2, // Disconnected at the user's request
    NETWORK_DISCONNECT_REASON_NETWORK_OFF = 3, // Disconnected due to the network shutdown
    NETWORK_DISCONNECT_REASON_LISTENING = 4, // Disconnected due to the listening mode
    NETWORK_DISCONNECT_REASON_SLEEP = 5, // Disconnected due to the sleep mode
    NETWORK_DISCONNECT_REASON_RESET = 6 // Disconnected to recover from cloud connection errors
} network_disconnect_reason;

typedef enum network_ready_type {
    NETWORK_READY_TYPE_ANY  = 0x00,
    NETWORK_READY_TYPE_IPV4 = 0x01,
    NETWORK_READY_TYPE_IPV6 = 0x02
} network_ready_type;
/**
 * network_handle_t used to differentiate between two networks
 * on the same device, e.g. WLAN and AP modes on Photon.
 */
typedef network_interface_t    network_handle_t;
const network_interface_t NIF_DEFAULT = 0;

/**
 * This is a bridge from the wiring layer to the system layer.
 * @return
 */

const void* network_config(network_handle_t network, uint32_t param1, void* reserved);

void network_connect(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved);
bool network_connecting(network_handle_t network, uint32_t param1, void* reserved);
void network_disconnect(network_handle_t network, uint32_t reason, void* reserved);
bool network_ready(network_handle_t network, uint32_t type, void* reserved);
void network_on(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved);
void network_off(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved);
int network_connect_cancel(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved);

#define NETWORK_LISTEN_EXIT (1<<0)
/**
 *
 * @param network
 * @param flags     NETWORK_LISTEN_EXIT bring the device out of listening mode
 * @param reserved
 */
void network_listen(network_handle_t network, uint32_t flags, void* reserved);
void network_set_listen_timeout(network_handle_t network, uint16_t timeout, void* reserved);
uint16_t network_get_listen_timeout(network_handle_t network, uint32_t flags, void* reserved);
bool network_listening(network_handle_t network, uint32_t param1, void* reserved);

typedef enum {
    NETWORK_LISTEN_COMMAND_NONE,
    NETWORK_LISTEN_COMMAND_ENTER,
    NETWORK_LISTEN_COMMAND_EXIT,
    NETWORK_LISTEN_COMMAND_CLEAR_CREDENTIALS
} network_listen_command_t;

int network_listen_command(network_handle_t network, network_listen_command_t command, void* arg);

bool network_has_credentials(network_handle_t network, uint32_t param1, void* reserved);

#include "wlan_hal.h"
typedef WLanCredentials NetworkCredentials;

/**
 *
 * @param network   The network to configure the credentials.
 * @param flags     Flags. set to 0.
 * @param creds     The credentials to set. Should not be NULL.
 * @param reserved  For future expansion. Set to NULL.
 * @return 0 on success.
 */
int network_set_credentials(network_handle_t network, uint32_t flags, NetworkCredentials* creds, void* reserved);
bool network_clear_credentials(network_handle_t network, uint32_t flags, NetworkCredentials* creds, void* reserved);

void network_setup(network_handle_t network, uint32_t flags, void* reserved);

int network_set_hostname(network_handle_t network, uint32_t flags, const char* hostname, void* reserved);
int network_get_hostname(network_handle_t network, uint32_t flags, char* buffer, size_t buffer_len, void* reserved);

/**
 * Disable automatic listening mode when no credentials are configured.
 */
const int WIFI_CONNECT_SKIP_LISTEN = 1;

#ifdef __cplusplus
}
#endif


#endif	/* SYSTEM_NETWORK_H */

