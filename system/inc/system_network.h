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
#include "system_defs.h"
#include "system_network_configuration.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum network_ready_type {
    NETWORK_READY_TYPE_ANY  = 0x00,
    NETWORK_READY_TYPE_IPV4 = 0x01,
    NETWORK_READY_TYPE_IPV6 = 0x02
} network_ready_type;
/**
 * network_handle_t used to differentiate between two networks
 * on the same device
 */
typedef network_interface_t    network_handle_t;
const network_interface_t NIF_DEFAULT = 0;

/**
 * This is a bridge from the wiring layer to the system layer.
 * @return
 */

const void* network_config(network_handle_t network, uint32_t param1, void* reserved);

void network_connect(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved);
// NOTE: this API may be called from an ISR
bool network_connecting(network_handle_t network, uint32_t param1, void* reserved);
void network_disconnect(network_handle_t network, uint32_t reason, void* reserved);
bool network_ready(network_handle_t network, uint32_t type, void* reserved);
void network_on(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved);
void network_off(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved);
bool network_is_on(network_handle_t network, void* reserved);
bool network_is_off(network_handle_t network, void* reserved);
int network_connect_cancel(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved);

#define NETWORK_LISTEN_EXIT (1<<0)
/**
 *
 * @param network
 * @param flags     NETWORK_LISTEN_EXIT bring the device out of listening mode
 * @param reserved
 */
void network_listen(network_handle_t network, uint32_t flags, void* reserved);
int network_listen_sync(network_handle_t network, uint32_t flags, void* reserved);
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

int network_wait_off(network_handle_t network, system_tick_t timeout, void*);

#if HAL_USE_SOCKET_HAL_POSIX
int network_set_configuration(network_handle_t network, const network_configuration_t* conf, void* reserved);
int network_get_configuration(network_handle_t network, network_configuration_t** conf, size_t* count, const char* profile, size_t profile_len, void* reserved);
int network_free_configuration(network_configuration_t* conf, size_t count, void* reserved);
#endif // HAL_USE_SOCKET_HAL_POSIX

/**
 * Disable automatic listening mode when no credentials are configured.
 */
const int WIFI_CONNECT_SKIP_LISTEN = 1;

#ifdef __cplusplus
}
#endif


#endif	/* SYSTEM_NETWORK_H */

