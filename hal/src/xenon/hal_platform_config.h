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

#ifndef HAL_PLATFORM_CONFIG_H
#define HAL_PLATFORM_CONFIG_H

#define HAL_PLATFORM_CLOUD_UDP (1)

#define HAL_PLATFORM_DCT (1)

#define HAL_USE_SOCKET_HAL_COMPAT (1)
#define HAL_USE_INET_HAL_COMPAT (1)

#define HAL_USE_SOCKET_HAL_POSIX (1)
#define HAL_USE_INET_HAL_POSIX (1)

#define HAL_PLATFORM_MESH (1)

#define HAL_PLATFORM_OPENTHREAD (1)

#define HAL_PLATFORM_LWIP (1)

#define HAL_PLATFORM_FILESYSTEM (1)

#define HAL_IPv6 (1)

#define HAL_PLATFORM_BLE         (1)

/* XXX: */
#define HAL_PLATFORM_DEFAULT_CLOUD_KEEPALIVE_INTERVAL (20000)

#endif  /* HAL_PLATFORM_CONFIG_H */
