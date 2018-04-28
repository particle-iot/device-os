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
 *  This file implements POSIX-compatible socket_hal for mesh-virtual platform.
 */

/* socket_hal_posix_impl.h should get included from socket_hal.h automagically */
#include "socket_hal.h"

int sock_accept(int s, struct sockaddr* addr, socklen_t* addrlen) {
  return lwip_accept(s, addr, addrlen);
}

int sock_bind(int s, const struct sockaddr* name, socklen_t namelen) {
  return lwip_bind(s, name, namelen);
}

int sock_shutdown(int s, int how) {
  return lwip_shutdown(s, how);
}

int sock_getpeername(int s, struct sockaddr* name, socklen_t* namelen) {
  return lwip_getpeername(s, name, namelen);
}

int sock_getsockname(int s, struct sockaddr* name, socklen_t* namelen) {
  return lwip_getsockname(s, name, namelen);
}

int sock_getsockopt(int s, int level, int optname, void* optval, socklen_t* optlen) {
  return lwip_getsockopt(s, level, optname, optval, optlen);
}

int sock_setsockopt(int s, int level, int optname, const void* optval, socklen_t optlen) {
  return lwip_setsockopt(s, level, optname, optval, optlen);
}

int sock_close(int s) {
  return lwip_close(s);
}

int sock_connect(int s, const struct sockaddr* name, socklen_t namelen) {
  return lwip_connect(s, name, namelen);
}

int sock_listen(int s, int backlog) {
  return lwip_listen(s, backlog);
}

ssize_t sock_recv(int s, void* mem, size_t len, int flags) {
  return lwip_recv(s, mem, len, flags);
}

ssize_t sock_recvfrom(int s, void* mem, size_t len, int flags,
                      struct sockaddr* from, socklen_t* fromlen) {
  return lwip_recvfrom(s, mem, len, flags, from, fromlen);
}

ssize_t sock_send(int s, const void* dataptr, size_t size, int flags) {
  return lwip_send(s, dataptr, size, flags);
}

ssize_t sock_sendto(int s, const void* dataptr, size_t size, int flags,
                    const struct sockaddr* to, socklen_t tolen) {
  return lwip_sendto(s, dataptr, size, flags, to, tolen);
}

int sock_socket(int domain, int type, int protocol) {
  return lwip_socket(domain, type, protocol);
}
