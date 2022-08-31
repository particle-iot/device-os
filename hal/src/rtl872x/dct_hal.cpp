/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "dct_hal.h"
#include "dct_file.h"

using particle::dct::DctFile;

DctFile& dct() {
    static DctFile dct;
    return dct;
}

const void* dct_read_app_data(uint32_t offset) {
    /* Deprecated */
    return nullptr;
}

int dct_read_app_data_copy(uint32_t offset, void* ptr, size_t size) {
    int result = -1;
    dct_lock(0);
    result = dct().read(offset, (uint8_t*)ptr, size);
    dct_unlock(0);
    return result > 0 ? 0 : result;
}

const void* dct_read_app_data_lock(uint32_t offset) {
    /* Deprecated */
    return nullptr;
}

int dct_read_app_data_unlock(uint32_t offset) {
    /* Deprecated */
    return -1;
}

int dct_write_app_data(const void* data, uint32_t offset, uint32_t size) {
    dct_lock(1);
    const int result = dct().write(offset, (const uint8_t*)data, size);
    dct_unlock(1);
    return result > 0 ? 0 : result;
}

int dct_clear() {
    dct_lock(0);
    const bool ok = dct().clear();
    dct_unlock(0);
    return (ok ? 0 : SYSTEM_ERROR_UNKNOWN);
}
