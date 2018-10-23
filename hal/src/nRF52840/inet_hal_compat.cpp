/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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
 */

#include "inet_hal_compat.h"
#include "system_error.h"
#include "net_hal.h"

int inet_gethostbyname(const char* hostname, uint16_t hostnameLen, HAL_IPAddress* out_ip_addr, network_interface_t nif, void* reserved)
{
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int inet_ping(const HAL_IPAddress* address, network_interface_t nif, uint8_t nTries, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

uint32_t HAL_NET_SetNetWatchDog(uint32_t) {
    return 0;
}

void HAL_NET_SetCallbacks(const HAL_NET_Callbacks* callbacks, void* reserved) {
}
