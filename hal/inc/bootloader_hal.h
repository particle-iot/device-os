/**
   Copyright (c) 2013-2016 Particle Industries, Inc.  All rights reserved.

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

#ifndef BOOTLOADER_HAL_H_
#define BOOTLOADER_HAL_H_

#include "hal_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#if !defined(SYSTEM_MINIMAL)
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
#define HAL_REPLACE_BOOTLOADER
#endif // PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
#endif // !defined(SYSTEM_MINIMAL)


const uint8_t* HAL_Bootloader_Image(uint32_t* size, void* reserved);

#ifdef __cplusplus
}
#endif

#endif  /* BOOTLOADER_HAL_H_ */

