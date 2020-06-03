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
 *  This file implements POSIX-compatible socket_hal for NCP-based platforms.
 */

/* socket_hal_posix_impl.h should get included from socket_hal.h automagically */
#include "socket_hal.h"
#include <cstdarg>

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
#ifdef LIBC_64_BIT_TIME_T
  if (optname == SO_SNDTIMEO || optname == SO_RCVTIMEO) {
    if (optlen && *optlen == sizeof(LIBC_TIMEVAL32) && optval) {
      struct timeval tv = {};
      socklen_t sz = sizeof(tv);
      int r = lwip_getsockopt(s, level, optname, &tv, &sz);
      if (!r) {
        LIBC_TIMEVAL32* tv32 = (LIBC_TIMEVAL32*)optval;
        tv32->tv_sec = tv.tv_sec;
        tv32->tv_usec = tv.tv_usec;
      }
      return r;
    }
  }
#endif // LIBC_64_BIT_TIME_T
  return lwip_getsockopt(s, level, optname, optval, optlen);
}

int sock_setsockopt(int s, int level, int optname, const void* optval, socklen_t optlen) {
#ifdef LIBC_64_BIT_TIME_T
  if (optname == SO_SNDTIMEO || optname == SO_RCVTIMEO) {
    if (optlen == sizeof(LIBC_TIMEVAL32) && optval) {
      LIBC_TIMEVAL32* tv32 = (LIBC_TIMEVAL32*)optval;
      struct timeval tv = {
        .tv_sec = tv32->tv_sec,
        .tv_usec = tv32->tv_usec
      };
      return lwip_setsockopt(s, level, optname, &tv, sizeof(tv));
    }
  }
#endif // LIBC_64_BIT_TIME_T
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

int sock_fcntl(int s, int cmd, ...) {
  va_list vl;
  va_start(vl, cmd);
  int val = va_arg(vl, int);
  va_end(vl);
  return lwip_fcntl(s, cmd, val);
}

int sock_poll(struct pollfd* fds, nfds_t nfds, int timeout) {
  return lwip_poll(fds, nfds, timeout);
}

int sock_select(int nfds, fd_set* readfds, fd_set* writefds,
                fd_set* exceptfds, struct timeval* timeout) {
  return lwip_select(nfds, readfds, writefds, exceptfds, timeout);
}

#ifdef LIBC_64_BIT_TIME_T
int sock_select32(int nfds, fd_set* readfds, fd_set* writefds,
                  fd_set* exceptfds, LIBC_TIMEVAL32* timeout) {
  struct timeval tv = {};
  if (timeout) {
    tv.tv_sec = timeout->tv_sec;
    tv.tv_usec = timeout->tv_usec;
    return sock_select(nfds, readfds, writefds, exceptfds, &tv);
  }
  return sock_select(nfds, readfds, writefds, exceptfds, nullptr);
}
#else
int sock_select32(int nfds, fd_set* readfds, fd_set* writefds,
                  fd_set* exceptfds, LIBC_TIMEVAL32* timeout) __attribute__((alias("sock_select")));
#endif // LIBC_64_BIT_TIME_T

ssize_t sock_recvmsg(int s, struct msghdr *message, int flags) {
  return lwip_recvmsg(s, message, flags);
}

ssize_t sock_sendmsg(int s, const struct msghdr *message, int flags) {
  return lwip_sendmsg(s, message, flags);
}

int sock_ioctl(int s, long cmd, void* argp) {
  return lwip_ioctl(s, cmd, argp);
}
