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

#ifndef HAL_DYNALIB_SOCKET_COMPAT_H
#define HAL_DYNALIB_SOCKET_COMPAT_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "socket_hal_compat.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_socket)

DYNALIB_FN(0, hal_socket, socket_active_status, uint8_t(sock_handle_t))
DYNALIB_FN(1, hal_socket, socket_handle_valid, uint8_t(sock_handle_t))
DYNALIB_FN(2, hal_socket, socket_create, sock_handle_t(uint8_t, uint8_t, uint8_t, uint16_t, network_interface_t))
DYNALIB_FN(3, hal_socket, socket_connect, int32_t(sock_handle_t, const sockaddr_t*, long))
DYNALIB_FN(4, hal_socket, socket_receive, sock_result_t(sock_handle_t, void*, socklen_t, system_tick_t))
DYNALIB_FN(5, hal_socket, socket_receivefrom, sock_result_t(sock_handle_t, void*, socklen_t, uint32_t, sockaddr_t*, socklen_t*))
DYNALIB_FN(6, hal_socket, socket_send, sock_result_t(sock_handle_t, const void*, socklen_t))
DYNALIB_FN(7, hal_socket, socket_sendto, sock_result_t(sock_handle_t, const void*, socklen_t, uint32_t, sockaddr_t*, socklen_t))
DYNALIB_FN(8, hal_socket, socket_close, sock_result_t(sock_handle_t))
DYNALIB_FN(9, hal_socket, socket_reset_blocking_call, sock_result_t(void))
DYNALIB_FN(10, hal_socket, socket_create_tcp_server, sock_result_t(uint16_t, network_interface_t))
DYNALIB_FN(11, hal_socket, socket_accept, sock_result_t(sock_handle_t))
DYNALIB_FN(12, hal_socket, socket_handle_invalid, sock_handle_t(void))
DYNALIB_FN(13, hal_socket, socket_join_multicast, sock_result_t(const HAL_IPAddress*, network_interface_t, socket_multicast_info_t*))
DYNALIB_FN(14, hal_socket, socket_leave_multicast, sock_result_t(const HAL_IPAddress*, network_interface_t, socket_multicast_info_t*))
DYNALIB_FN(15, hal_socket, socket_peer, sock_result_t(sock_handle_t, sock_peer_t*, void*))
DYNALIB_FN(16, hal_socket, socket_shutdown, sock_result_t(sock_handle_t, int))
DYNALIB_FN(17, hal_socket, socket_send_ex, sock_result_t(sock_handle_t, const void*, socklen_t, uint32_t, system_tick_t, void*))

DYNALIB_END(hal_socket)

#endif /* HAL_DYNALIB_SOCKET_COMPAT_H */
