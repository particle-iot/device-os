/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "storage_hal.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_storage)

DYNALIB_FN(0, hal_storage, hal_storage_read, int(hal_storage_id, uintptr_t, uint8_t*, size_t))
DYNALIB_FN(1, hal_storage, hal_storage_write, int(hal_storage_id, uintptr_t, const uint8_t*, size_t))
DYNALIB_FN(2, hal_storage, hal_storage_erase, int(hal_storage_id, uintptr_t, size_t))

DYNALIB_END(hal_storage)
