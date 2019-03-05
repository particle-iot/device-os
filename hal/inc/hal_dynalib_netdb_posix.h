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

#ifndef HAL_DYNALIB_NETDB_POSIX_H
#define HAL_DYNALIB_NETDB_POSIX_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "netdb_hal.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_netdb)

DYNALIB_FN(0, hal_netdb, netdb_gethostbyname, struct hostent*(const char*))
DYNALIB_FN(1, hal_netdb, netdb_gethostbyname_r, int(const char*, struct hostent*, char*, size_t, struct hostent**, int*))
DYNALIB_FN(2, hal_netdb, netdb_freeaddrinfo, void(struct addrinfo*))
DYNALIB_FN(3, hal_netdb, netdb_getaddrinfo, int(const char*, const char*, const struct addrinfo*, struct addrinfo**))
DYNALIB_FN(4, hal_netdb, netdb_getnameinfo, int(const struct sockaddr*, socklen_t, char*, socklen_t, char*, socklen_t, int))

DYNALIB_END(hal_netdb)

#endif /* HAL_DYNALIB_NETDB_POSIX_H */
