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

#ifndef GNSS_HAL_H
#define GNSS_HAL_H

#include "hal_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#if HAL_PLATFORM_GNSS

int hal_gnss_init(void* reserved);
int hal_gnss_pos(void* reserved);

#endif // HAL_PLATFORM_GNSS

#ifdef __cplusplus
}
#endif

#endif  /* GNSS_HAL_H */
