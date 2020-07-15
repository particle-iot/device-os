/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include "system_error.h"

// Note: This API is is not thread-safe.

namespace particle::system {

/**
 * Set the last error message.
 */
void setErrorMessage(const char* msg);
/**
 * Format the last error message.
 */
void formatErrorMessage(const char* fmt, ...);
/**
 * Get the last error message.
 */
const char* errorMessage();
/**
 * Get the last error message.
 *
 * This method returns a default code-specific message if the last error message is not set.
 */
const char* errorMessage(int code);
/**
 * Clear the last error message.
 */
void clearErrorMessage();

} // namespace particle::system
