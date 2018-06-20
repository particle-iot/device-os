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
 *  This file defines the POSIX-compatible netdb_hal and associated types.
 */

#ifndef NETDB_HAL_H
#define NETDB_HAL_H

#include "netdb_hal_impl.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef EAI_SYSTEM
#define EAI_SYSTEM 254
#endif /* EAI_SYSTEM */

#ifndef EAI_BADFLAGS
#define EAI_BADFLAGS (-1)
#endif /* EAI_BADFLAGS */

#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST AI_NUMERICHOST
#endif /* NI_NUMERICHOST */

#ifndef NI_NUMERICSERV
#define NI_NUMERICSERV AI_NUMERICSERV
#endif /* NI_NUMERICHOST */

/**
 * Gets the IPv4 address for the given hostname.
 *
 * @param[in]  name  the hostname
 *
 * @returns a pointer to a structure of type hostent or NULL
 */
struct hostent* netdb_gethostbyname(const char *name);

/**
 * Re-entrant variant of netdb_gethostbyname()
 *
 * @param[in]  name      the hostname
 * @param[out] ret       hostent structure
 * @param      buf       temporary work buffer
 * @param[in]  buflen    the length of the temporary work buffer
 * @param[out] result    points to a resulting hostent structure on success
 *                       or is set to NULL on error
 * @param[in]  h_errnop  the pointer to a variable to store the error number
 *
 * @returns    0 on success or non-zero error code in case of failure.
 */
int netdb_gethostbyname_r(const char* name, struct hostent* ret, char* buf,
                          size_t buflen, struct hostent** result, int* h_errnop);

/**
 * Frees the addrinfo structure allocated in netdb_getaddrinfo()
 *
 * @param      ai    addrinfo struct to free
 */
void netdb_freeaddrinfo(struct addrinfo* ai);

/**
 * Get a list of IP addresses and port numbers for host hostname and service servname.
 *
 * @param[in]  hostname  the hostname
 * @param[in]  servname  the service name
 * @param[in]  hints     the hints
 * @param[out] res       on success, set to a pointer to a linked list of one or more addrinfo structures
 *
 * @returns    0 on success or non-zero error code in case of failure.
 */
int netdb_getaddrinfo(const char* hostname, const char* servname,
                      const struct addrinfo* hints, struct addrinfo** res);

/**
 * Converts a sockaddr structure to a pair of host name and service strings.
 *
 * @param[in]  sa       pointer to sockaddr structure
 * @param[in]  salen    sa length in bytes
 * @param[out] host     the resulting host name
 * @param[in]  hostlen  the length of the host buffer
 * @param[out] serv     the resulting service name
 * @param[in]  servlen  the length of the serv buffer
 * @param[in]  flags    a combination of NI_NOFQDN, NI_NUMERICHOST, NI_NAMEREQD, NI_NUMERICSERV and NI_DGRAM
 *
 * @return     { description_of_the_return_value }
 */
int netdb_getnameinfo(const struct sockaddr* sa, socklen_t salen, char* host,
                      socklen_t hostlen, char* serv, socklen_t servlen, int flags);

/**
 * @}
 *
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NETDB_HAL_H */
