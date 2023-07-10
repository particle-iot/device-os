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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdint.h>
#include "nrfx_qspi.h"

// Default flash type to support at least standard flash commands
#define HAL_QSPI_DEFAULT_SPI_FLASH_TYPE                      HAL_QSPI_FLASH_TYPE_MX25R6435F

#define HAL_QSPI_FLASH_OTP_SECTOR_SIZE_MX25R6435F   1024
#define HAL_QSPI_FLASH_OTP_SECTOR_SIZE_MX25L3233F   512
#define HAL_QSPI_FLASH_OTP_SECTOR_COUNT_GD25        3
#define HAL_QSPI_FLASH_OTP_SECTOR_SIZE_GD25         1024
#define HAL_QSPI_FLASH_OTP_SIZE_GD25                (HAL_QSPI_FLASH_OTP_SECTOR_SIZE_GD25 * HAL_QSPI_FLASH_OTP_SECTOR_COUNT_GD25)

typedef enum hal_qspi_flash_type_t {
    HAL_QSPI_FLASH_TYPE_UNKNOWN,
    HAL_QSPI_FLASH_TYPE_MX25L3233F,
    HAL_QSPI_FLASH_TYPE_MX25R6435F,
    HAL_QSPI_FLASH_TYPE_GD25WQ64E
} hal_qspi_flash_type_t;

typedef enum hal_qspi_flash_cmd_t {
    HAL_QSPI_CMD_STD_WRSR            = 0x01, // Write Status Register
    HAL_QSPI_CMD_STD_WREN            = 0x06, // Write Enable: Sets WEL bit before every write operation
    HAL_QSPI_CMD_STD_RSTEN           = 0x66, // Enable Reset
    HAL_QSPI_CMD_STD_RST             = 0x99, // Reset
    HAL_QSPI_CMD_STD_SLEEP           = 0xB9, // GD25: "Power Down", MX25: "Deep Power Down"
    HAL_QSPI_CMD_STD_READ_ID         = 0x9F, // Read Identification
    HAL_QSPI_CMD_STD_READ_4IO        = 0xEB, // Quad data line SPI Read
    HAL_QSPI_CMD_STD_WRITE_PP4O      = 0x32, // Quad data line SPI Write (address bits sent on IO 0)
    HAL_QSPI_CMD_STD_WRITE_PP4IO     = 0x38, // Quad data line SPI Write (address bits sent on IOs 0-3)

    HAL_QSPI_CMD_MX25_ENSO           = 0xB1, // Enter Secured OTP
    HAL_QSPI_CMD_MX25_EXSO           = 0xC1, // Exit Secured OTP
    HAL_QSPI_CMD_MX25_RDSCUR         = 0x2B, // Read Security Register
    HAL_QSPI_CMD_MX25_WRSCUR         = 0x2F, // Write Security Register (ie lock OTP)
    HAL_QSPI_CMD_MX25_PGMERS_SUSPEND = 0xB0, // Program/Erase Suspend

    HAL_QSPI_CMD_GD25_WRSR2          = 0x31, // Write Status Register 2
    HAL_QSPI_CMD_GD25_WRSR3          = 0x11, // Write Status Register 3
    HAL_QSPI_CMD_GD25_SEC_ERASE      = 0x44, // Security Register Erase (ie Erase OTP)
    HAL_QSPI_CMD_GD25_SEC_READ       = 0x48, // Security Register Read
    HAL_QSPI_CMD_GD25_SEC_PROGRAM    = 0x42, // Security Register Program (ie Write OTP)
    HAL_QSPI_CMD_GD25_PGMERS_SUSPEND = 0x75, // Program/Erase Suspend
    HAL_QSPI_CMD_GD25_WAKE           = 0xAB, // Release from Deep Power-Down and Read Device ID
} hal_qspi_flash_cmd_t;

typedef struct  {
    hal_qspi_flash_type_t   type;            // hal_qspi_flash_type_t to identify what part is present
    hal_qspi_flash_cmd_t    write_opcode;    // Which opcode to use for quad write
    hal_qspi_flash_cmd_t    read_opcode;     // Which opcode to use for quad read
    uint8_t                 suspend_opcode;  // Which opcode to use when interrupting pending program/erase command
    uint8_t                 reserved[3];     // Reserved padding
    uint32_t                otp_size;        // Size of OTP area in bytes
    uint32_t                size;            // Size of flash in bytes
} hal_exflash_params_t;

typedef int (*hal_exflash_gd25_security_register_read_write) (uint8_t, uint32_t, void *, size_t);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

hal_qspi_flash_type_t hal_exflash_get_type(const uint8_t* id_buf);
const hal_exflash_params_t* hal_exflash_get_params(hal_qspi_flash_type_t type);

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */
