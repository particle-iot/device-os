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
 *  This file implements POSIX-compatible netdb_hal for mesh-virtual platform.
 */

/* netdb_hal_impl.h should get included from netdb_hal.h automagically */
#include "netdb_hal.h"
#include <errno.h>

struct hostent* netdb_gethostbyname(const char *name) {
  return lwip_gethostbyname(name);
}

int netdb_gethostbyname_r(const char* name, struct hostent* ret, char* buf,
                          size_t buflen, struct hostent** result, int* h_errnop) {
  return lwip_gethostbyname_r(name, ret, buf, buflen, result, h_errnop);
}

void netdb_freeaddrinfo(struct addrinfo* ai) {
  return lwip_freeaddrinfo(ai);
}

int netdb_getaddrinfo(const char* hostname, const char* servname,
                      const struct addrinfo* hints, struct addrinfo** res) {
  return lwip_getaddrinfo(hostname, servname, hints, res);
}

int netdb_getnameinfo(const struct sockaddr* sa, socklen_t salen, char* host,
                      socklen_t hostlen, char* serv, socklen_t servlen, int flags) {
  /* Not implemented */
  errno = ENOSYS;
  return EAI_SYSTEM;
}
