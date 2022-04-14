/**
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

#pragma once

#include <stddef.h>

/**
 * Callback to add interpolated variables to a buffer.
 */
typedef size_t (*string_interpolate_source_t)(const char* ident, size_t ident_len, char* buf, size_t buf_len);

/**
 * Interpolates variable placeholders in a buffer.
 * Each variable starts with $ and is a sequence of one or more alphanumeric characters, plus underscore.
 */
size_t system_string_interpolate(const char* source, char* dest, size_t dest_len, string_interpolate_source_t vars);
