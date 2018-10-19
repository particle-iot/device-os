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
 *  This file defines the implementation details for POSIX-compatible inet_hal for mesh-virtual platform.
 */

#ifndef INET_HAL_POSIX_IMPL_H
#define INET_HAL_POSIX_IMPL_H

#include <lwip/sockets.h>
#include <lwip/inet.h>

#ifndef IN6_IS_ADDR_V4MAPPED
#define IN6_IS_ADDR_V4MAPPED(a) \
    ((((uint32_t*)(a))[0] == 0) && (((uint32_t*)(a))[1] == 0) && \
     (((uint32_t*)(a))[2] == htonl(0xffff)))
#endif // IN6_IS_ADDR_V4MAPPED

#ifndef IN6_IS_ADDR_MULTICAST
#define IN6_IS_ADDR_MULTICAST(a) (((uint8_t *)(a))[0] == 0xff)
#endif // IN6_IS_ADDR_MULTICAST

#ifndef IN6_IS_ADDR_LINKLOCAL
#define IN6_IS_ADDR_LINKLOCAL(a) \
    ((((uint32_t*)(a))[0] & htonl(0xffc00000)) == htonl(0xfe800000))
#endif // IN6_IS_ADDR_LINKLOCAL

#ifndef IN6_IS_ADDR_LOOPBACK
#define IN6_IS_ADDR_LOOPBACK(a) \
    (((uint32_t*)(a))[0] == 0 && ((uint32_t*)(a))[1] == 0 && \
     ((uint32_t*)(a))[2] == 0 && ((uint32_t*)(a))[3] == htonl(1))
#endif // IN6_IS_ADDR_LOOPBACK

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @addtogroup inet_hal_posix_impl
 *
 * @brief
 *   This module provides implementation details for POSIX-compatible inet_hal for mesh-virtual platform.
 *
 * @{
 *
 */

/**
 * @}
 *
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INET_HAL_POSIX_IMPL_H */
