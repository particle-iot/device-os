/**
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

#include "ota_flash_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Checks if the minimum required dependencies for the given module are satisfied.
 * @param bounds    The bounds of the module to check.
 * @return {@code true} if the dependencies are satisfied, {@code false} otherwise.
 */
bool validate_module_dependencies(const module_bounds_t* bounds, bool userPartOptional, bool fullDeps);
const module_bounds_t* find_module_bounds(uint8_t module_function, uint8_t module_index);
bool fetch_module(hal_module_t* target, const module_bounds_t* bounds, bool userDepsOptional, uint16_t check_flags);
const module_info_t* locate_module(const module_bounds_t* bounds);

#ifdef __cplusplus
}
#endif
