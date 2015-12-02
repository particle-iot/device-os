/**
 ******************************************************************************
 * @file    inet_hal.c
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    25-Sept-2014
 * @brief
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


#include "inet_hal.h"
#include "socket.h"
#include "delay_hal.h"

int inet_gethostbyname(const char* hostname, uint16_t hostnameLen, HAL_IPAddress* out_ip_addr,
    network_interface_t nif, void* reserved)
{
    int attempts = 5;
    out_ip_addr->ipv4 = 0;
    int result = 0;
    while (!out_ip_addr->ipv4 && attempts --> 0) {
        HAL_Delay_Milliseconds(1);
        result = gethostbyname(hostname, hostnameLen, &out_ip_addr->ipv4)<0;
    }
    return result;
}

// inet_ping in wlan_hal.c
