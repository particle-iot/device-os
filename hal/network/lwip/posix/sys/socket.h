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

/*
 * Inline functions instead of function-like macros where the names commonly
 * appear as struct members or methods in portable code (a function-like
 * macro breaks any 'obj->name(...)' call expression).
 */
static inline int accept(int s, struct sockaddr* addr, socklen_t* addrlen) {
    return sock_accept(s, addr, addrlen);
}
static inline int connect(int s, const struct sockaddr* name, socklen_t namelen) {
    return sock_connect(s, name, namelen);
}
static inline int listen(int s, int backlog) {
    return sock_listen(s, backlog);
}
static inline ssize_t recv(int s, void* mem, size_t len, int flags) {
    return sock_recv(s, mem, len, flags);
}
static inline ssize_t send(int s, const void* dataptr, size_t size, int flags) {
    return sock_send(s, dataptr, size, flags);
}

static inline int bind(int s, const struct sockaddr* name, socklen_t namelen) {
    return sock_bind(s, name, namelen);
}
static inline int shutdown(int s, int how) {
    return sock_shutdown(s, how);
}
static inline int getpeername(int s, struct sockaddr* name, socklen_t* namelen) {
    return sock_getpeername(s, name, namelen);
}
static inline int getsockname(int s, struct sockaddr* name, socklen_t* namelen) {
    return sock_getsockname(s, name, namelen);
}
static inline int getsockopt(int s, int level, int optname, void* optval, socklen_t* optlen) {
    return sock_getsockopt(s, level, optname, optval, optlen);
}
static inline int setsockopt(int s, int level, int optname, const void* optval, socklen_t optlen) {
    return sock_setsockopt(s, level, optname, optval, optlen);
}
static inline ssize_t recvfrom(int s, void* mem, size_t len, int flags, struct sockaddr* from, socklen_t* fromlen) {
    return sock_recvfrom(s, mem, len, flags, from, fromlen);
}
static inline ssize_t sendto(int s, const void* dataptr, size_t size, int flags, const struct sockaddr* to, socklen_t tolen) {
    return sock_sendto(s, dataptr, size, flags, to, tolen);
}
static inline int socket(int domain, int type, int protocol) {
    return sock_socket(domain, type, protocol);
}
static inline int poll(struct pollfd* fds, nfds_t nfds, int timeout) {
    return sock_poll(fds, nfds, timeout);
}
static inline ssize_t recvmsg(int s, struct msghdr* message, int flags) {
    return sock_recvmsg(s, message, flags);
}
static inline ssize_t sendmsg(int s, const struct msghdr* message, int flags) {
    return sock_sendmsg(s, message, flags);
}

#define close(s) sock_close(s)
#define select(nfds, readfds, writefds, exceptfds, timeout) sock_select(nfds, readfds, writefds, exceptfds, timeout)

#endif /* SYS_SOCKET_H */
