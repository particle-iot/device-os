/*
 * Copyright (c) 2025 Particle Industries, Inc.  All rights reserved.
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

#include "hal_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#if HAL_PLATFORM_ENV_VARS

int system_get_env_var(const char* name, char* buf, size_t buf_size, int* found, void* reserved);
int system_get_env_var_names(const char* names[], size_t names_size, void* reserved);

#endif // HAL_PLATFORM_ENV_VARS

#ifdef __cplusplus
} // extern "C"
#endif
