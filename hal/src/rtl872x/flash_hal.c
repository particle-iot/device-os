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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "flash_mal.h"
#include "flash_hal.h"
#include "flash_acquire.h"
#include "flash_common.h"
#include "exflash_hal.h"
#include "rtl8721d.h"

int hal_flash_init(void) {
    return 0;
}

int hal_flash_write(uintptr_t addr, const uint8_t* data_buf, size_t data_size) {
    return hal_exflash_write(addr - SPI_FLASH_BASE, data_buf, data_size);
}

int hal_flash_erase_sector(uintptr_t addr, size_t num_sectors) {
    return hal_exflash_erase_sector(addr - SPI_FLASH_BASE, num_sectors);
}

int hal_flash_read(uintptr_t addr, uint8_t* data_buf, size_t size) {
    return hal_exflash_read(addr - SPI_FLASH_BASE, data_buf, size);
}

int hal_flash_copy_sector(uintptr_t src_addr, uintptr_t dest_addr, size_t data_size) {
    return hal_exflash_copy_sector(src_addr - SPI_FLASH_BASE, dest_addr - SPI_FLASH_BASE, data_size);
}
