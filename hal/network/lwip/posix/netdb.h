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
 *  This is a POSIX wrapper for netdb_hal
 */

#ifndef NETDB_H
#define NETDB_H

#include "netdb_hal.h"

#define gethostbyname(name) netdb_gethostbyname(name)
#define gethostbyname_r(name, ret, buf, buflen, result, h_errnop) netdb_gethostbyname_r(name, ret, buf, buflen, result, h_errnop)
#define freeaddrinfo(ai) netdb_freeaddrinfo(ai)
#define getaddrinfo(hostname, servname, hints, res) netdb_getaddrinfo(hostname, servname, hints, res)
#define getnameinfo(sa, salen, host, hostlen, serv, servlen, flags) netdb_getnameinfo(sa, salen, host, hostlen, serv, servlen, flags)

#endif /* NETDB_H */
