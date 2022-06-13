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
        .write_opcode   = NRF_QSPI_WRITEOC_PP4IO,
        .read_opcode    = NRF_QSPI_READOC_READ4IO,
        .suspend_opcode = HAL_QSPI_CMD_MX25_PGMERS_SUSPEND,
        .reserved       = {},
        .otp_size       = MX25L3233F_OTP_SECTOR_SIZE
    },
    {
        .type           = HAL_QSPI_FLASH_TYPE_MX25R6435F,
        .write_opcode   = NRF_QSPI_WRITEOC_PP4IO,
        .read_opcode    = NRF_QSPI_READOC_READ4IO,
        .suspend_opcode = HAL_QSPI_CMD_MX25_PGMERS_SUSPEND,
        .reserved       = {},
        .otp_size       = MX25R6435F_OTP_SECTOR_SIZE
    },
    {
        .type           = HAL_QSPI_FLASH_TYPE_GD25WQ64E,
        .write_opcode   = NRF_QSPI_WRITEOC_PP4O,
        .read_opcode    = NRF_QSPI_READOC_READ4IO,
        .suspend_opcode = HAL_QSPI_CMD_GD25_PGMERS_SUSPEND,
        .reserved       = {},
        .otp_size       = GD25_SECURITY_REGISTER_SIZE
    }
};

} // Anonymous

const hal_exflash_params_t* hal_exflash_get_params(hal_qspi_flash_type_t type) {
    CHECK_TRUE(type != HAL_QSPI_FLASH_TYPE_UNKNOWN, nullptr);
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
