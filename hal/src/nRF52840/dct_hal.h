/*
 ******************************************************************************
 *  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
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
 ******************************************************************************
 */
#pragma once

#include <stdint.h>
#include <stddef.h>

#include "dct.h"

#include "security_mode.h"

#ifdef __cplusplus
extern "C" {
#endif

SECURITY_MODE_PROTECTED_FN(int, dct_read_app_data_copy, (uint32_t offset, void* ptr, size_t size));
SECURITY_MODE_PROTECTED_FN(int, dct_write_app_data,(const void* data, uint32_t offset, uint32_t size));
int dct_lock(int write);
int dct_unlock(int write);

int dct_clear();

#ifdef __cplusplus
} // extern "C"
#endif
