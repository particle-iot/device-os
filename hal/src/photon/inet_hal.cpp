/**
 ******************************************************************************
 * @file    inet_hal.cpp
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    31-Oct-2014
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
#include "wiced_tcpip.h"

int inet_gethostbyname(const char* hostname, uint16_t hostnameLen, HAL_IPAddress* out_ip_addr, network_interface_t nif, void* reserved)
{
    wiced_ip_address_t address;
    wiced_result_t result = wiced_hostname_lookup (hostname, &address, 5000);
    if (result == WICED_SUCCESS) {
    		HAL_IPV4_SET(out_ip_addr, GET_IPV4_ADDRESS(address));
    }
    return -result;
}

int inet_ping(const HAL_IPAddress* address, network_interface_t nif, uint8_t nTries, void* reserved) {

    const uint32_t     ping_timeout = 1000;
    uint32_t           elapsed_ms;
    wiced_ip_address_t ping_target_ip;

    SET_IPV4_ADDRESS(ping_target_ip, address->ipv4);

    int count = 0;
    for (int i=0; i<nTries; i++) {
        wiced_result_t     status = wiced_ping(WICED_STA_INTERFACE, &ping_target_ip, ping_timeout, &elapsed_ms);
        if (status==WICED_SUCCESS)
            count++;
    }
    return count;
}
