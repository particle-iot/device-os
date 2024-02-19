/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

#include <string.h>

#include "hal_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#if PLATFORM_ID == PLATFORM_GCC && defined(__GLIBC__)

size_t strlcpy(char* dest, const char* src, size_t size);

#endif // PLATFORM_ID == PLATFORM_GCC && defined(__GLIBC__)

#ifdef __cplusplus
} // extern "C"
#endif
