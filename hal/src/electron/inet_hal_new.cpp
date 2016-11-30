/*
 ******************************************************************************
 *  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef HAL_CELLULAR_EXCLUDE

#include "inet_hal.h"
#include "parser.h"

int inet_gethostbyname(const char* hostname, uint16_t hostnameLen, HAL_IPAddress* out_ip_addr,
		network_interface_t nif, void* reserved)
{
	uint32_t result = electronMDM.gethostbyname(hostname);
	if (result > 0) {
		out_ip_addr->ipv4 = result;
		return 0;
	}
    return 1;
}

int inet_ping(const HAL_IPAddress* address, network_interface_t nif, uint8_t nTries,
        void* reserved)
{
    // Replace with AT_COMMAND_HAL implementation
    return 0;
}

#endif // !defined(HAL_CELLULAR_EXCLUDE)
