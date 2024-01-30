/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

#include "efuse.h"
#include <memory>
#include <cstring>
#include "system_error.h"
#include "core_hal.h"
#include "module_info.h"
#include "check.h"

extern "C" {
#include "rtl8721d.h"
#include "rtl8721d_efuse.h"
}

int efuse_read_logical(uint32_t offset, uint8_t* buf, size_t size) {
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    std::unique_ptr<uint8_t[]> efuseData(new uint8_t[EFUSE_LOGICAL_SIZE]);
    CHECK_TRUE(efuseData, SYSTEM_ERROR_NO_MEMORY);
    uint8_t* efuseBuf = efuseData.get();
#else
    // No heap in bootloader
    static uint8_t efuseBuf[EFUSE_LOGICAL_SIZE];
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

    memset(efuseBuf, 0xff, EFUSE_LOGICAL_SIZE);

    bool dataConsistent = false;
    for (int i = 0; i < 5 && !dataConsistent; i++) {
        EFUSE_LMAP_READ(efuseBuf);
        uint32_t crc1 = HAL_Core_Compute_CRC32(efuseBuf, EFUSE_LOGICAL_SIZE);

        EFUSE_LMAP_READ(efuseBuf);
        uint32_t crc2 = HAL_Core_Compute_CRC32(efuseBuf, EFUSE_LOGICAL_SIZE);

        dataConsistent = (crc1 == crc2);
    }

    if (!dataConsistent) {
        return SYSTEM_ERROR_INTERNAL;
    }
    memcpy(buf, efuseBuf + offset, size);

    return SYSTEM_ERROR_NONE;
}

int efuse_write_logical(uint32_t offset, const uint8_t* buf, size_t size) {
    if (EFUSE_LMAP_WRITE(offset, size, (uint8_t*)buf) != EFUSE_SUCCESS) {
        return SYSTEM_ERROR_FLASH_IO;
    }

    return 0;
}
