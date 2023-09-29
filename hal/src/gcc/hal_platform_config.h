/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#pragma once

#include "platforms.h"


#ifndef HAL_PLATFORM_USART_9BIT_SUPPORTED
#define HAL_PLATFORM_USART_9BIT_SUPPORTED (1)
#endif

#define HAL_PLATFORM_CLOUD_UDP 1
#define HAL_PLATFORM_CLOUD_TCP 1

#ifndef HAL_PLATFORM_WIFI
#define HAL_PLATFORM_WIFI 0
#endif

#ifndef HAL_PLATFORM_CELLULAR
#define HAL_PLATFORM_CELLULAR 0
#endif

#ifndef HAL_PLATFORM_CLOUD_UDP
#define HAL_PLATFORM_CLOUD_UDP 0
#endif

#ifndef HAL_PLATFORM_CLOUD_TCP
#define HAL_PLATFORM_CLOUD_TCP 0
#endif

#ifndef HAL_PLATFORM_NCP
#define HAL_PLATFORM_NCP 	(0)
#endif

#ifndef HAL_PLATFORM_NCP_AT
#define HAL_PLATFORM_NCP_AT (0)
#endif

#define HAL_PLATFORM_USB_CDC (1)

#define HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME (1*60*1000)

#if HAL_PLATFORM_WIFI
#define HAL_PLATFORM_NETWORK_MULTICAST (1)
#endif

#define PRODUCT_SERIES "gcc"

#define HAL_PLATFORM_SPI_NUM (2)
#define HAL_PLATFORM_I2C_NUM (1)
#define HAL_PLATFORM_USART_NUM (5)

#if defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE
#define HAL_PLATFORM_MULTIPART_SYSTEM 1
#endif

#define HAL_PLATFORM_FREERTOS (0)
