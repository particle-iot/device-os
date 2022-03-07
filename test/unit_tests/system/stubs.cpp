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

#include "ota_flash_hal.h"
#include "diagnostics.h"

int HAL_System_Info(hal_system_info_t* info, bool construct, void* reserved) {
    return 0;
}

int diag_enum_sources(diag_enum_sources_callback callback, size_t* count, void* data, void* reserved) {
    return 0;
}

int diag_get_source(uint16_t id, const diag_source** src, void* reserved) {
    return 0;
}