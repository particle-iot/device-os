/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "exflash_hal_params.h"
#include "check.h"

namespace {

constexpr uint32_t MX25_MANUFACTURER_ID   = 0xC2;
constexpr uint32_t MX25R6435F_MEMORY_TYPE = 0x28;
constexpr uint32_t MX25L3233F_MEMORY_TYPE = 0x20;
constexpr uint32_t GD25_MANUFACTURER_ID   = 0xC8;

constexpr hal_exflash_params_t exflash_params[] = {
    {
        .type           = HAL_QSPI_FLASH_TYPE_MX25L3233F,
        .write_opcode   = HAL_QSPI_CMD_STD_WRITE_PP4IO,
        .read_opcode    = HAL_QSPI_CMD_STD_READ_4IO,
        .suspend_opcode = HAL_QSPI_CMD_MX25_PGMERS_SUSPEND,
        .reserved       = {},
        .otp_size       = HAL_QSPI_FLASH_OTP_SECTOR_SIZE_MX25L3233F,
        .size           = 4 * 1024 * 1024 // 4MB
    },
    {
        .type           = HAL_QSPI_FLASH_TYPE_MX25R6435F,
        .write_opcode   = HAL_QSPI_CMD_STD_WRITE_PP4IO,
        .read_opcode    = HAL_QSPI_CMD_STD_READ_4IO,
        .suspend_opcode = HAL_QSPI_CMD_MX25_PGMERS_SUSPEND,
        .reserved       = {},
        .otp_size       = HAL_QSPI_FLASH_OTP_SECTOR_SIZE_MX25R6435F,
        .size           = 8 * 1024 * 1024 // 8MB
    },
    {
        .type           = HAL_QSPI_FLASH_TYPE_GD25WQ64E,
        .write_opcode   = HAL_QSPI_CMD_STD_WRITE_PP4O,
        .read_opcode    = HAL_QSPI_CMD_STD_READ_4IO,
        .suspend_opcode = HAL_QSPI_CMD_GD25_PGMERS_SUSPEND,
        .reserved       = {},
        .otp_size       = HAL_QSPI_FLASH_OTP_SIZE_GD25,
        .size           = 8 * 1024 * 1024 // 8MB
    }
};

} // Anonymous

const hal_exflash_params_t* hal_exflash_get_params(hal_qspi_flash_type_t type) {
    if (type == HAL_QSPI_FLASH_TYPE_UNKNOWN) {
        type = HAL_QSPI_DEFAULT_SPI_FLASH_TYPE;
    }
    int index = static_cast<int>(type) - 1;
    return &exflash_params[index];
}

hal_qspi_flash_type_t hal_exflash_get_type(const uint8_t* id_buf) {
    if (id_buf[0] == MX25_MANUFACTURER_ID && id_buf[1] == MX25L3233F_MEMORY_TYPE) {
        return HAL_QSPI_FLASH_TYPE_MX25L3233F;
    } else if (id_buf[0] == MX25_MANUFACTURER_ID && id_buf[1] == MX25R6435F_MEMORY_TYPE) {
        return HAL_QSPI_FLASH_TYPE_MX25R6435F;
    } else if (id_buf[0] == GD25_MANUFACTURER_ID) {
        return HAL_QSPI_FLASH_TYPE_GD25WQ64E;
    }
    return HAL_QSPI_FLASH_TYPE_UNKNOWN;
}
