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

#if HAL_PLATFORM_ENV_VARS

#include <stdbool.h>

typedef enum {
	SYSTEM_ENV_NEED_RESET = 1
} system_env_result;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get the value of an environment variable.
 *
 * The output is always null-terminated unless the size of the output buffer is 0.
 *
 * @param name Variable name.
 * @param buf Output buffer.
 * @param buf_size Size of the output buffer.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return On success, the actual length of the variable value, not including `\0`, otherwise an
 *         error code defined by `system_error_t`. The returned length can be greater than the size
 *         of the output buffer. Returns `SYSTEM_ERROR_ENV_NOT_FOUND` if the variable is not defined.
 */
int system_get_env(const char* name, char* buf, size_t buf_size, void* reserved);

/**
 * Get the value of an environment variable and convert it to an integer.
 *
 * Only decimal digits with an optional leading minus sign are valid.
 *
 * @param name Variable name.
 * @param[out] val Output value. Only modified if the variable is defined and valid.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 if the variable is defined and contains a valid integer, otherwise an error code
 *         defined by `system_error_t`. Returns `SYSTEM_ERROR_ENV_NOT_FOUND` if the variable is not
 *         defined, or `SYSTEM_ERROR_ENV_INVALID_VALUE` if the value is not a valid integer or is
 *         not in the range representable by `int`.
 */
int system_get_env_int(const char* name, int* val, void* reserved);

/**
 * Get the value of an environment variable and convert it to a boolean.
 *
 * Only `true` and `false` (case-sensitive, lowercase only) are valid.
 *
 * @param name Variable name.
 * @param[out] val Output value. Only modified if the variable is defined and valid.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 if the variable is defined and contains a valid boolean value, otherwise an error code
 *         defined by `system_error_t`. Returns `SYSTEM_ERROR_ENV_NOT_FOUND` if the variable is not
 *         defined, or `SYSTEM_ERROR_ENV_INVALID_VALUE` if the value is not a valid boolean.
 */
int system_get_env_bool(const char* name, bool* val, void* reserved);

/**
 * List all defined environment variables.
 *
 * @param[out] names Array to store the variable names.
 * @param count Maximum number of elements that can be stored in the array.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return On success, the actual number of defined variables, otherwise an error code defined by
 *         `system_error_t`. The returned number of variables can be greater than the size of the
 *         output array.
 */
int system_list_env(const char* names[], size_t count, void* reserved);

/**
 * Clear all defined environment variables.
 *
 * The variables will be cleared next time the device boots.
 *
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 or `SYSTEM_ENV_NEED_RESET` on success, otherwise an error code defined by
 *         `system_error_t`. `SYSTEM_ENV_NEED_RESET` indicates that a system reset is needed for
 *         the changes to take effect.
 */
int system_clear_env(void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HAL_PLATFORM_ENV_VARS
