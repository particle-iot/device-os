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
#include "flash_mal.h"
#include "platform_config.h"
#include "system_error.h"
#include "spi_flash.h"
#include "flash_mal.h"
#include "check.h"
#include <algorithm>

int hal_storage_read(hal_storage_id id, uintptr_t addr, uint8_t* buf, size_t size) {
    if (id == HAL_STORAGE_ID_INTERNAL_FLASH) {
        memcpy(buf, (void*)addr, size);
        return size;
    }
#ifdef USE_SERIAL_FLASH
    else if (id == HAL_STORAGE_ID_EXTERNAL_FLASH) {
        sFLASH_ReadBuffer(buf, addr, size);
        return size;
    }
#endif // USE_SERIAL_FLASH

    return SYSTEM_ERROR_NOT_FOUND;
}

int hal_storage_write(hal_storage_id id, uintptr_t addr, const uint8_t* buf, size_t size) {
    if (id == HAL_STORAGE_ID_INTERNAL_FLASH) {
        FLASH_Unlock();
        FLASH_ClearFlags();
        /* Data received are Word multiple */
        FLASH_Status status = FLASH_COMPLETE;
        for (size_t pos = 0; pos < size && status == FLASH_COMPLETE; pos += sizeof(uint32_t)) {
            uint32_t tmp = 0xffffff;
            memcpy(&tmp, buf + pos, std::min(sizeof(tmp), size - pos));
            status = FLASH_ProgramWord(addr + pos, tmp);
        }
        FLASH_Lock();
        CHECK_TRUE(status == FLASH_COMPLETE, SYSTEM_ERROR_FLASH_IO);
        return size;
    }
#ifdef USE_SERIAL_FLASH
    else if (id == HAL_STORAGE_ID_EXTERNAL_FLASH) {
        sFLASH_WriteBuffer(buf, addr, size);
        return size;
    }
#endif // USE_SERIAL_FLASH

    return SYSTEM_ERROR_NOT_FOUND;
}

int hal_storage_erase(hal_storage_id id, uintptr_t addr, size_t size) {
    if (id == HAL_STORAGE_ID_INTERNAL_FLASH) {
        CHECK_TRUE(FLASH_EraseMemory(FLASH_INTERNAL, addr, size), SYSTEM_ERROR_FLASH_IO);
        // FIXME: returned value needs to be aligned to the sector size
        return size;
    }
#ifdef USE_SERIAL_FLASH
    else if (id == HAL_STORAGE_ID_EXTERNAL_FLASH) {
        CHECK_TRUE(FLASH_EraseMemory(FLASH_SERIAL, addr, size), SYSTEM_ERROR_FLASH_IO);
        // FIXME: returned value needs to be aligned to the sector size
        return size;
    }
#endif // USE_SERIAL_FLASH

    return SYSTEM_ERROR_NOT_FOUND;
}
