/**
 ******************************************************************************
 * @file    inet_hal.cpp
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    31-Oct-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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
#include "wiced_tcpip.h"

int inet_gethostbyname(char* hostname, uint16_t hostnameLen, uint32_t* out_ip_addr)
{
    wiced_ip_address_t address;
    wiced_result_t result = wiced_hostname_lookup (hostname, &address, 1000);
    return result;
}

// inet_ping in wlan_hal.c