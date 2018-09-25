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
 *  This file defines the POSIX-compatible inet_hal and associated types.
 */

#ifndef INET_HAL_POSIX_H
#define INET_HAL_POSIX_H

#include "inet_hal_posix_impl.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @addtogroup inet_hal
 *
 * @brief
 *   This module provides POSIX-compatible inet functions.
 *
 * @{
 *
 */

/**
 * Converts a string containing an IPv4 dotted-decimal address into in_addr_t.
 *
 * @param[in]  cp    a string containing IPv4 address
 *
 * @returns    in_addr_t representation of an IPv4 address (network byte order),
 *             or INADDR_NONE in case of an error
 */
in_addr_t inet_inet_addr(const char* cp);

/**
 * Converts a string containing an IPv4 dotted-decimal address into struct in_addr.
 *
 * @param[in]  cp    a string containing IPv4 address
 * @param[out] pin   struct in_addr that will contain the binary representation of the IPv4 address
 *                   in network byte order
 *
 * @returns    0 on success, non-zero error code on error.
 */
int inet_inet_aton(const char* cp, struct in_addr* pin);

/**
 * Converts a string containing an IPv4 dotted-decimal address into in_addr_t.
 *
 * @param[in]  cp    a string containing IPv4 address
 *
 * @returns    in_addr_t representation of an IPv4 address (host byte order),
 *             or INADDR_NONE in case of an error
 */
in_addr_t inet_inet_network(const char* cp);

/**
 * Converts an address in in network byte order to a string representation.
 *
 * @param[in]  in    address in network byte order
 *
 * @returns    String representation of in IPv4 address.
 *
 */
char* inet_inet_ntoa(struct in_addr in);

/**
 * Re-entrant variant of inet_inet_ntoa()
 *
 * @param[in]  in    address in network byte order
 * @param[out] buf   output buffer
 * @param[in]  size  the size of the output buffer
 *
 * @returns    String representation of in IPv4 address.
 */
char* inet_inet_ntoa_r(struct in_addr in, char *buf, socklen_t size);

/**
 * Converts IPv4 and IPv6 addresses from binary to text format
 *
 * @param[in]  af    address family (AF_INET or AF_INET6)
 * @param[in]  src   address to convert in binary format
 * @param[out] dst   output buffer
 * @param[in]  size  the size of the output buffer
 *
 * @returns    String representation of IPv4 or IPv6 address.
 */
const char* inet_inet_ntop(int af, const void* src, char* dst, socklen_t size);

/**
 * Converts IPv4 and IPv6 addresses from text to binary format
 *
 * @param[in]  af    address family (AF_INET or AF_INET6)
 * @param[in]  src   string representation of an IPv4 or IPv6 address
 * @param[out] dst   pointer to an address structure
 *
 * @returns    1 on success, 0 is returned if src does not contain a character
 *             string representing a valid network address in the specified address family.
 *             If af does not contain a valid address family, -1 is returned.
 */
int inet_inet_pton(int af, const char* src, void* dst);

/**
 * Converts the unsigned integer from host byte order to network byte order.
 *
 * @param[in]  v     input unsigned integer
 *
 * @returns    unsigned integer in network byte order
 */
uint32_t inet_htonl(uint32_t v);

/**
 * Converts the unsigned integer from network byte order to host byte order.
 *
 * @param[in]  v     input unsigned integer
 *
 * @returns    unsigned integer in host byte order
 */
uint32_t inet_ntohl(uint32_t v);

/**
 * Converts the unsigned short integer from host byte order to network byte order.
 *
 * @param[in]  v     input unsigned short integer
 *
 * @returns    unsigned short integer in network byte order
 */
uint16_t inet_htons(uint16_t v);

/**
 * Converts the unsigned short integer from network byte order to host byte order.
 *
 * @param[in]  v     input unsigned short integer
 *
 * @returns    unsigned short integer in host byte order
 */
uint16_t inet_ntohs(uint16_t v);

/**
 * @}
 *
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INET_HAL_POSIX_H */
