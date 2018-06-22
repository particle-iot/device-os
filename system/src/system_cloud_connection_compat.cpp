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

#include "hal_platform.h"

/* TODO: untested */

#if !HAL_USE_SOCKET_HAL_POSIX && HAL_USE_SOCKET_HAL_COMPAT

#include "system_cloud_connection.h"
#include "system_cloud_internal.h"
#include "system_cloud.h"

namespace {

sock_handle_t s_cloudSocket = socket_handle_invalid();

} /* anonymous */

int system_cloud_connect(int protocol, const ServerAddress* address, sockaddr* saddrCache)
{
    return -1;
}

int system_cloud_disconnect(int flags)
{
    return -1;
}

int system_cloud_send(const uint8_t* buf, size_t buflen, int flags)
{
    return -1;
}

int system_cloud_recv(uint8_t* buf, size_t buflen, int flags)
{
    return -1;
}

int system_internet_test(void* reserved)
{
    return -1;
}

int system_multicast_announce_presence(void* reserved)
{
    return -1;
}

int system_cloud_is_connected(void* reserved)
{
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

#ifndef SPARK_NO_CLOUD
void HAL_NET_notify_socket_closed(sock_handle_t socket)
{
    if (s_cloudSocket == socket)
    {
        cloud_disconnect(false, false, CLOUD_DISCONNECT_REASON_ERROR);
    }
}
#else
void HAL_NET_notify_socket_closed(sock_handle_t socket)
{
}
#endif /* SPARK_NO_CLOUD */

#endif /* !HAL_USE_SOCKET_HAL_POSIX && HAL_USE_SOCKET_HAL_COMPAT */
