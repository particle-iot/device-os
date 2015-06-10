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

#ifndef SPARK_NO_WIFI
#define SPARK_NO_WIFI 0
#endif    
    
#if SPARK_NO_WIFI
#undef SPARK_NO_CLOUD
#define SPARK_NO_CLOUD 1    
#endif
    
typedef network_interface_t    network_handle_t;
const network_interface_t NIF_DEFAULT = 0;
    
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



#ifdef __cplusplus
}
#endif


#endif	/* SYSTEM_NETWORK_H */

