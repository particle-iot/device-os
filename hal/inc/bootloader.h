/*
 * Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
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

#ifndef BOOTLOADER_H
#define	BOOTLOADER_H

#include "module_info.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

bool bootloader_requires_update(const uint8_t* bootloader_image, uint32_t length);
bool bootloader_update_if_needed();

int bootloader_update(const void* bootloader_image, unsigned length);

// TODO: We already have Bootloader_Get_Version() in the platform code, but that function
// works via the system flags, which is less reliable than reading the version number directly.
// Make sure we have only one function to retrieve the bootloader version going forward.
uint16_t bootloader_get_version(void);

#ifdef __cplusplus
}
#endif

#endif	/* BOOTLOADER_H */

