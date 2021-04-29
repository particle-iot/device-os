/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include "module_info.h"
#include "ota_flash_hal.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * Augments the module info with radio stack module info.
 */
int platform_radio_stack_fetch_module_info(hal_module_t* module);

#ifdef __cplusplus
}
#endif // __cplusplus
