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

/**
 * Get the value of an environment variable.
 *
 * The output is null-terminated unless the requested variable is not defined or the size of the
 * output buffer is 0.
 *
 * @param name Variable name.
 * @param buf Output buffer.
 * @param buf_size Size of the output buffer.
 * @param[out] found If not `NULL`, the argument will be set to a non-zero value if the variable
 *             is defined, otherwise to 0.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return On success, the actual length of the variable value, not including `\0`, or 0 if the
 *         variable is not defined. On failure, an error code defined by `system_error_t`. The
 *         returned length can be greater than the size of the output buffer. 
 */
int system_get_env_var(const char* name, char* buf, size_t buf_size, int* found, void* reserved);

/**
 * Get the names of all defined environment variables.
 *
 * @param[out] names Array to store the variable names.
 * @param names_size Maximum number of elements that can be stored in the array.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return On success, the actual number of defined variables. On failure, an error code defined by
 *         `system_error_t`. The returned number of variables can be greater than the size of the
 *         output array.
 */
int system_list_env_vars(const char* names[], size_t names_size, void* reserved);

#endif // HAL_PLATFORM_ENV_VARS

#ifdef __cplusplus
} // extern "C"
#endif
