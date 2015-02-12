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

const WLanConfig& network_config();

void network_connect();
bool network_connecting();
bool network_connected();
void network_disconnect();
bool network_ready();
void network_on();
void network_off(bool disconnect_cloud);
void network_listen();
bool network_listening();
bool network_has_credentials();

struct NetworkCredentials : WLanCredentials {
    
};

void network_set_credentials(NetworkCredentials* creds);
bool network_clear_credentials();


#endif	/* SYSTEM_NETWORK_H */

