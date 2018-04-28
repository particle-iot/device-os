/**
 ******************************************************************************
 * @file    socket_hal.c
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

#include "socket_hal.h"


int32_t socket_connect(sock_handle_t sd, const sockaddr_t *addr, long addrlen)
{
    return 0;
}

sock_result_t socket_reset_blocking_call()
{
    return 0;
}

sock_result_t socket_receive(sock_handle_t sd, void* buffer, socklen_t len, system_tick_t _timeout)
{
  return 0;
}

sock_result_t socket_create_nonblocking_server(sock_handle_t sock, uint16_t port)
{
    return 0;
}

sock_result_t socket_receivefrom(sock_handle_t sock, void* buffer, socklen_t bufLen, uint32_t flags, sockaddr_t* addr, socklen_t* addrsize)
{
    return 0;
}

sock_result_t socket_bind(sock_handle_t sock, uint16_t port)
{
    return 0;
}

sock_result_t socket_accept(sock_handle_t sock)
{
    return 0;
}

uint8_t socket_active_status(sock_handle_t socket)
{
    return 0;
}

sock_result_t socket_close(sock_handle_t sock)
{
    return 0;
}

sock_result_t socket_shutdown(sock_handle_t sd, int how)
{
    return -1;
}

sock_handle_t socket_create(uint8_t family, uint8_t type, uint8_t protocol, uint16_t port, network_interface_t nif)
{
    return 0;
}

sock_result_t socket_send(sock_handle_t sd, const void* buffer, socklen_t len)
{
    return 0;
}

sock_result_t socket_send_ex(sock_handle_t sd, const void* buffer, socklen_t len, uint32_t flags, system_tick_t timeout, void* reserved)
{
    return 0;
}

sock_result_t socket_sendto(sock_handle_t sd, const void* buffer, socklen_t len, uint32_t flags, sockaddr_t* addr, socklen_t addr_size)
{
    return 0;
}

const sock_handle_t SOCKET_MAX = (sock_handle_t)0xFFFE;
const sock_handle_t SOCKET_INVALID = (sock_handle_t)-1;

inline bool is_valid(sock_handle_t handle) {
    return handle<SOCKET_MAX;
}

uint8_t socket_handle_valid(sock_handle_t handle) {
    return is_valid(handle);
}

sock_handle_t socket_handle_invalid()
{
    return SOCKET_INVALID;
}


sock_result_t socket_join_multicast(const HAL_IPAddress* addr, network_interface_t nif, socket_multicast_info_t* reserved)
{
    return -1;
}

sock_result_t socket_leave_multicast(const HAL_IPAddress* addr, network_interface_t nif, socket_multicast_info_t* reserved)
{
	return -1;
}

sock_result_t socket_peer(sock_handle_t sd, sock_peer_t* peer, void* reserved)
{
    return -1;
}
