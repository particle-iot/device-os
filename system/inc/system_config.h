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
 * The output is always null-terminated unless the size of the output buffer is 0.
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
 * List all defined environment variables.
 *
 * @param[out] names Array to store the variable names.
 * @param count Maximum number of elements that can be stored in the array.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return On success, the actual number of defined variables. On failure, an error code defined by
 *         `system_error_t`. The returned number of variables can be greater than the size of the
 *         output array.
 */
int system_list_env_vars(const char* names[], size_t count, void* reserved);

/**
 * Clear all defined environment variables.
 *
 * The variables will be cleared next time the device boots.
 *
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by `system_error_t`.
 */
int system_clear_env_vars(void* reserved);

/**
 * Get the value of an environment variable and validate it as a boolean.
 *
 * Only "true" and "false" (case-sensitive, lowercase only) are valid.
 *
 * @param name Variable name.
 * @param[out] val Output boolean value. Only modified if the variable is defined and valid.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 if the variable is defined and contains a valid boolean value, otherwise an error code
 *         defined by `system_error_t`. Returns SYSTEM_ERROR_NOT_FOUND if not defined, or
 *         SYSTEM_ERROR_BAD_DATA if the value is not a valid boolean.
 */
int system_get_env_var_bool(const char* name, bool* val, void* reserved);

/**
 * Get the value of an environment variable and validate it as a 32-bit signed integer.
 *
 * Only decimal digits with an optional leading minus sign are valid.
 * Overflow is detected and returns an error.
 *
 * @param name Variable name.
 * @param[out] val Output integer value. Only modified if the variable is defined and valid.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 if the variable is defined and contains a valid integer, otherwise an error code
 *         defined by `system_error_t`. Returns SYSTEM_ERROR_NOT_FOUND if not defined, or
 *         SYSTEM_ERROR_BAD_DATA if the value is not a valid integer or overflows.
 */
int system_get_env_var_int(const char* name, int* val, void* reserved);

#endif // HAL_PLATFORM_ENV_VARS

#ifdef __cplusplus
} // extern "C"
#endif
