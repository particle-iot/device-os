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

#ifndef HAL_DYNALIB_INET_POSIX_H
#define HAL_DYNALIB_INET_POSIX_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "inet_hal_posix.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_inet)

DYNALIB_FN(0, hal_inet, inet_inet_addr, in_addr_t(const char*))
DYNALIB_FN(1, hal_inet, inet_inet_aton, int(const char*, struct in_addr*))
DYNALIB_FN(2, hal_inet, inet_inet_network, in_addr_t(const char*))
DYNALIB_FN(3, hal_inet, inet_inet_ntoa, char*(struct in_addr))
DYNALIB_FN(4, hal_inet, inet_inet_ntoa_r, char*(struct in_addr, char*, socklen_t))
DYNALIB_FN(5, hal_inet, inet_inet_ntop, const char*(int, const void*, char*, socklen_t))
DYNALIB_FN(6, hal_inet, inet_inet_pton, int(int, const char*, void*))
DYNALIB_FN(7, hal_inet, inet_ntohl, uint32_t(uint32_t))
DYNALIB_FN(8, hal_inet, inet_htonl, uint32_t(uint32_t))
DYNALIB_FN(9, hal_inet, inet_ntohs, uint16_t(uint16_t))
DYNALIB_FN(10, hal_inet, inet_htons, uint16_t(uint16_t))

DYNALIB_END(hal_inet)

#endif /* HAL_DYNALIB_INET_POSIX_H */
