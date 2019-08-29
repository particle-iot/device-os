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
#ifndef HAL_DYNALIB_SOCKET_POSIX_H
#define HAL_DYNALIB_SOCKET_POSIX_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "socket_hal_posix.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_socket)

DYNALIB_FN(0, hal_socket, sock_accept, int(int, struct sockaddr*, socklen_t*))
DYNALIB_FN(1, hal_socket, sock_bind, int(int, const struct sockaddr*, socklen_t))
DYNALIB_FN(2, hal_socket, sock_shutdown, int(int, int))
DYNALIB_FN(3, hal_socket, sock_getpeername, int(int, struct sockaddr*, socklen_t*))
DYNALIB_FN(4, hal_socket, sock_getsockname, int(int, struct sockaddr*, socklen_t*))
DYNALIB_FN(5, hal_socket, sock_getsockopt, int(int, int, int, void*, socklen_t*))
DYNALIB_FN(6, hal_socket, sock_setsockopt, int(int, int, int, const void*, socklen_t))
DYNALIB_FN(7, hal_socket, sock_close, int(int))
DYNALIB_FN(8, hal_socket, sock_connect, int(int, const struct sockaddr*, socklen_t))
DYNALIB_FN(9, hal_socket, sock_listen, int(int, int))
DYNALIB_FN(10, hal_socket, sock_recv, int(int, void*, size_t, int))
DYNALIB_FN(11, hal_socket, sock_recvfrom, int(int, void*, size_t, int, struct sockaddr*, socklen_t*))
DYNALIB_FN(12, hal_socket, sock_send, int(int, const void*, size_t, int))
DYNALIB_FN(13, hal_socket, sock_sendto, int(int, const void*, size_t, int, const struct sockaddr*, socklen_t))
DYNALIB_FN(14, hal_socket, sock_socket, int(int, int, int))
DYNALIB_FN(15, hal_socket, sock_fcntl, int(int, int, ...))
DYNALIB_FN(16, hal_socket, sock_poll, int(struct pollfd*, nfds_t, int))
DYNALIB_FN(17, hal_socket, sock_select, int(int, fd_set*, fd_set*, fd_set*, struct timeval*))

DYNALIB_END(hal_socket)

#endif /* HAL_DYNALIB_SOCKET_POSIX_H */
