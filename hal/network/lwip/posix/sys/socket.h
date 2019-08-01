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

/**
 * @file
 * @brief
 *  This is a POSIX wrapper for socket_hal_posix
 */

#ifndef SYS_SOCKET_H
#define SYS_SOCKET_H

#include "socket_hal.h"

#define accept(s, addr, addrlen) sock_accept(s, addr, addrlen)
#define bind(s, name, namelen) sock_bind(s, name, namelen)
#define shutdown(s, how) sock_shutdown(s, how)
#define getpeername(s, name, namelen) sock_getpeername(s, name, namelen)
#define getsockname(s, name, namelen) sock_getsockname(s, name, namelen)
#define getsockopt(s, level, optname, optval, optlen) sock_getsockopt(s, level, optname, optval, optlen)
#define setsockopt(s, level, optname, optval, optlen) sock_setsockopt(s, level, optname, optval, optlen)
#define close(s) sock_close(s)
#define connect(s, name, namelen) sock_connect(s, name, namelen)
#define listen(s, backlog) sock_listen(backlog)
#define recv(s, mem, len, flags) sock_recv(s, mem, len, flags)
#define recvfrom(s, mem, len, flags, from, fromlen) sock_recvfrom(s, mem, len, flags, from, fromlen)
#define send(s, dataptr, size, flags) sock_send(s, dataptr, size, flags)
#define sendto(s, dataptr, size, flags, to, tolen) sock_sendto(s, dataptr, size, flags, to, tolen)
#define socket(domain, type, protocol) sock_socket(domain, type, protocol)
#define poll(fds, nfds, timeout) sock_poll(fds, nfds, timeout)
#define select(maxfdp1, readset, writeset, exceptset, timeout) sock_select(maxfdp1, readset, writeset, exceptset, timeout)

#endif /* SYS_SOCKET_H */
