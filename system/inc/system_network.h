/**
 ******************************************************************************
 * @file    system_network.h
 * @authors Matthew McGowan
 * @date    12 February 2015
 ******************************************************************************
  Copyright (c) 2015 Spark Labs, Inc.  All rights reserved.

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

enum eWanTimings
{
    CONNECT_TO_ADDRESS_MAX = S2M(30),
    DISCONNECT_TO_RECONNECT = S2M(30),
};
    
    
extern volatile uint8_t WLAN_DISCONNECT;
extern volatile uint8_t WLAN_DHCP;
extern volatile uint8_t WLAN_MANUAL_CONNECT;
extern volatile uint8_t WLAN_DELETE_PROFILES;
extern volatile uint8_t WLAN_SMART_CONFIG_START;
extern volatile uint8_t WLAN_SMART_CONFIG_FINISHED;
extern volatile uint8_t WLAN_SERIAL_CONFIG_DONE;
extern volatile uint8_t WLAN_SMART_CONFIG_STOP;
extern volatile uint8_t WLAN_CAN_SHUTDOWN;

extern volatile uint8_t SPARK_WLAN_RESET;
extern volatile uint8_t SPARK_WLAN_SLEEP;
extern volatile uint8_t SPARK_WLAN_STARTED;

extern volatile uint8_t SPARK_LED_FADE;

    
typedef uint32_t    network_handle_t;
    
/**
 * This is a bridge from the wiring layer to the system layer.
 * @return 
 */
    
const WLanConfig* network_config(network_handle_t network, uint32_t param1, void* reserved);

void network_connect(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved);
bool network_connecting(network_handle_t network, uint32_t param1, void* reserved);
void network_disconnect(network_handle_t network, uint32_t param1, void* reserved);
bool network_ready(network_handle_t network, uint32_t param1, void* reserved);
void network_on(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved);
void network_off(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved);

void network_listen(network_handle_t network, uint32_t flags, void* reserved);
bool network_listening(network_handle_t network, uint32_t param1, void* reserved);


bool network_has_credentials(network_handle_t network, uint32_t param1, void* reserved);

typedef WLanCredentials NetworkCredentials;
    
void network_set_credentials(network_handle_t network, uint32_t flags, NetworkCredentials* creds, void* reserved);
bool network_clear_credentials(network_handle_t network, uint32_t flags, NetworkCredentials* creds, void* reserved);


void manage_smart_config();
void manage_ip_config();


#ifdef __cplusplus
}
#endif


#endif	/* SYSTEM_NETWORK_H */

