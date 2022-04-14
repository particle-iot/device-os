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
 *  This file defines the implementation details for POSIX-compatible socket_hal for NCP-based platforms.
 */

#ifndef SOCKET_HAL_POSIX_IMPL_H
#define SOCKET_HAL_POSIX_IMPL_H

#include <lwip/sockets.h>
#include "time_compat.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @addtogroup socket_hal_posix_impl
 *
 * @brief
 *   This module provides implementation details for POSIX-compatible socket_hal for NCP-based platforms.
 *
 * @{
 *
 */

#define AF_LINK     18
#define AF_PACKET   AF_LINK
#define PF_LINK     AF_LINK
#define PF_PACKET   AF_PACKET

struct sockaddr_ll {
    u8_t sll_len;
    sa_family_t sll_family;
    u16_t sll_protocol;
    uint8_t sll_ifindex;
    u16_t sll_hatype;
    u8_t sll_pkttype;
    u8_t sll_halen;
    u8_t sll_addr[8];
};

#ifdef LIBC_64_BIT_TIME_T
int sock_select32(int nfds, fd_set* readfds, fd_set* writefds,
                  fd_set* exceptfds, LIBC_TIMEVAL32* timeout);
#endif // LIBC_64_BIT_TIME_T

/**
 * @}
 *
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SOCKET_HAL_POSIX_IMPL_H */
