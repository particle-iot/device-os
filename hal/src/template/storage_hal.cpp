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

#include "storage_hal.h"
#include "system_error.h"

int hal_storage_read(hal_storage_id id, uintptr_t addr, uint8_t* buf, size_t size) {
    return SYSTEM_ERROR_NOT_FOUND;
}

int hal_storage_write(hal_storage_id id, uintptr_t addr, const uint8_t* buf, size_t size) {
    return SYSTEM_ERROR_NOT_FOUND;
}

int hal_storage_erase(hal_storage_id id, uintptr_t addr, size_t size) {
    return SYSTEM_ERROR_NOT_FOUND;
}
