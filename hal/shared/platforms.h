/**
 ******************************************************************************
 * @file    platforms.h
 * @authors Matthew McGowan, Brett Walach
 * @date    02 February 2015
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef PLATFORMS_H
#define	PLATFORMS_H

#define PLATFORM_GCC                        3
#define PLATFORM_PHOTON_DEV                 4
#define PLATFORM_TEACUP_PIGTAIL_DEV         5
#define PLATFORM_PHOTON_PRODUCTION          6
#define PLATFORM_TEACUP_PIGTAIL_PRODUCTION  7
#define PLATFORM_P1                         8
#define PLATFORM_ETHERNET_PROTO             9
#define PLATFORM_ELECTRON_PRODUCTION        10
#define PLATFORM_ARGON                      12
#define PLATFORM_BORON                      13
#define PLATFORM_XENON                      14
#define PLATFORM_BSOM                       23
#define PLATFORM_B5SOM                      25
#define PLATFORM_NEWHAL                     60000

#if PLATFORM_ID == PLATFORM_GCC
#define PRODUCT_SERIES                      "gcc"
#endif

#if PLATFORM_ID == PLATFORM_PHOTON_DEV || \
    PLATFORM_ID == PLATFORM_TEACUP_PIGTAIL_DEV || \
    PLATFORM_ID == PLATFORM_PHOTON_PRODUCTION || \
    PLATFORM_ID == PLATFORM_TEACUP_PIGTAIL_PRODUCTION || \
    PLATFORM_ID == PLATFORM_P1 || \
    PLATFORM_ID == PLATFORM_ETHERNET_PROTO
#define PRODUCT_SERIES                      "photon"
#endif

#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
#define PRODUCT_SERIES                      "electron"
#endif

#if PLATFORM_ID == PLATFORM_ARGON
#define PRODUCT_SERIES                      "argon"
#endif

#if PLATFORM_ID == PLATFORM_BORON || \
    PLATFORM_ID == PLATFORM_BSOM || \
    PLATFORM_ID == PLATFORM_B5SOM
#define PRODUCT_SERIES                      "boron"
#endif

#if PLATFORM_ID == PLATFORM_XENON
#define PRODUCT_SERIES                      "xenon"
#endif

#if PLATFORM_ID == PLATFORM_NEWHAL
#define PRODUCT_SERIES                      "newhal"
#endif

#endif	/* PLATFORMS_H */

