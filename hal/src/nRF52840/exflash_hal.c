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
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "exflash_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
/* nordic_common.h already defines MIN()/MAX() */
// #include <sys/param.h>
#include "nrfx_qspi.h"
#include "nrf_delay.h"
#include "app_util_platform.h"
#include "app_error.h"
#include "hw_config.h"
#include "logging.h"
#include "flash_common.h"

#define QSPI_STD_CMD_WRSR           0x01
#define QSPI_STD_CMD_RSTEN          0x66
#define QSPI_STD_CMD_RST            0x99

static int configure_memory()
{
    uint8_t temporary = 0x40;
    uint32_t err_code;
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode    = QSPI_STD_CMD_RSTEN,
        .length    = NRF_QSPI_CINSTR_LEN_1B,
        .io2_level = true,
        .io3_level = true,
        .wipwait   = true,
        .wren      = true
    };

    // Send reset enable
    err_code = nrfx_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);
    if (err_code)
    {
        return -1;
    }

    // Send reset command
    cinstr_cfg.opcode = QSPI_STD_CMD_RST;
    err_code = nrfx_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);
    if (err_code)
    {
        return -2;
    }

    // Switch to qspi mode
    cinstr_cfg.opcode = QSPI_STD_CMD_WRSR;
    cinstr_cfg.length = NRF_QSPI_CINSTR_LEN_2B;
    err_code = nrfx_qspi_cinstr_xfer(&cinstr_cfg, &temporary, NULL);
    if (err_code)
    {
        return -3;
    }

    return 0;
}

static int perform_write(uintptr_t addr, const uint8_t* data, size_t size) {
    return nrfx_qspi_write(data, size, addr);
}

int hal_exflash_init(void)
{
    int ret = 0;

    hal_exflash_lock();

    nrfx_qspi_config_t config = {
        .xip_offset  = NRFX_QSPI_CONFIG_XIP_OFFSET,
        .pins = {
           .sck_pin     = QSPI_FLASH_SCK_PIN,
           .csn_pin     = QSPI_FLASH_CSN_PIN,
           .io0_pin     = QSPI_FLASH_IO0_PIN,
           .io1_pin     = QSPI_FLASH_IO1_PIN,
           .io2_pin     = QSPI_FLASH_IO2_PIN,
           .io3_pin     = QSPI_FLASH_IO3_PIN,
        },
        .irq_priority   = (uint8_t)QSPI_FLASH_IRQ_PRIORITY,
        .prot_if = {
            .readoc     = (nrf_qspi_readoc_t)NRFX_QSPI_CONFIG_READOC,
            .writeoc    = (nrf_qspi_writeoc_t)NRFX_QSPI_CONFIG_WRITEOC,
            .addrmode   = (nrf_qspi_addrmode_t)NRFX_QSPI_CONFIG_ADDRMODE,
            .dpmconfig  = false,
        },
        .phy_if = {
            .sck_freq   = (nrf_qspi_frequency_t)NRFX_QSPI_CONFIG_FREQUENCY,
            .sck_delay  = (uint8_t)NRFX_QSPI_CONFIG_SCK_DELAY,
            .spi_mode   = (nrf_qspi_spi_mode_t)NRFX_QSPI_CONFIG_MODE,
            .dpmen      = false
        },
    };

    ret = nrfx_qspi_init(&config, NULL, NULL);
    if (ret)
    {
        goto hal_exflash_init_done;
    }
    LOG_DEBUG(TRACE, "QSPI initialized.");

    ret = configure_memory();
    if (ret)
    {
        goto hal_exflash_init_done;
    }

hal_exflash_init_done:
    hal_exflash_unlock();
    return 0;
}

int hal_exflash_write(uintptr_t addr, const uint8_t* data_buf, size_t data_size)
{
    hal_exflash_lock();
    int ret = hal_flash_common_write(addr, data_buf, data_size,
                                     &perform_write, &hal_flash_common_dummy_read);

    hal_exflash_unlock();
    return ret;
}

int hal_exflash_read(uintptr_t addr, uint8_t* data_buf, size_t data_size)
{
    int ret = 0;
    hal_exflash_lock();

    {
        const uintptr_t src_aligned = ADDR_ALIGN_WORD(addr);
        uint8_t* dst_aligned = (uint8_t*)ADDR_ALIGN_WORD_RIGHT((uintptr_t)data_buf);
        const size_t size_aligned = ADDR_ALIGN_WORD((data_size - (dst_aligned - data_buf)));

        if (size_aligned > 0) {
            /* Read-out the aligned portion into an aligned address in the buffer
             * with an adjusted size in multiples of 4.
             */
            ret = nrfx_qspi_read(dst_aligned, size_aligned, src_aligned);

            if (ret != NRF_SUCCESS) {
                goto hal_exflash_read_done;
            }

            /* Move the data if necessary */
            if (dst_aligned != data_buf || src_aligned != addr) {
                const unsigned offset_front = addr - src_aligned;
                const size_t bytes_read = size_aligned - offset_front;

                memmove(data_buf, dst_aligned + offset_front, bytes_read);

                /* Adjust addresses and size */
                addr += bytes_read;
                data_buf += bytes_read;
                data_size -= bytes_read;
            } else {
                /* Adjust addresses and size */
                addr += size_aligned;
                data_buf += size_aligned;
                data_size -= size_aligned;
            }
        }
    }

    /* Read the remaining unaligned portion into a temporary buffer and copy into
     * the destination buffer
     */
    if (data_size > 0) {
        uint8_t tmpbuf[sizeof(uint32_t) * 2] __attribute__((aligned(4)));

        /* Read the remainder */
        const uintptr_t src_aligned = ADDR_ALIGN_WORD(addr);
        const size_t size_aligned = ADDR_ALIGN_WORD_RIGHT((addr + data_size) - src_aligned);

        ret = nrfx_qspi_read(tmpbuf, size_aligned, src_aligned);

        if (ret != NRF_SUCCESS) {
            goto hal_exflash_read_done;
        }

        const unsigned offset_front = addr - src_aligned;

        memcpy(data_buf, tmpbuf + offset_front, data_size);
    }

hal_exflash_read_done:
    hal_exflash_unlock();
    return ret;
}

static int erase_common(uintptr_t start_addr, size_t num_blocks, nrf_qspi_erase_len_t len) {
    hal_exflash_lock();
    int err_code = NRF_SUCCESS;

    const size_t block_length = len == QSPI_ERASE_LEN_LEN_4KB ? 4096 : 64 * 1024;

    /* Round down to the nearest block */
    start_addr = ((start_addr / block_length) * block_length);

    for (int i = 0; i < num_blocks; i++)
    {
        err_code = nrfx_qspi_erase(len, start_addr);
        if (err_code)
        {
            goto erase_common_done;
        }
    }

    LOG_DEBUG(TRACE, "Erased %lu %lukB blocks starting from %" PRIxPTR,
              num_blocks, block_length / 1024, start_addr);

erase_common_done:
    hal_exflash_unlock();
    return err_code;
}

int hal_exflash_erase_sector(uintptr_t start_addr, size_t num_sectors)
{
    return erase_common(start_addr, num_sectors, QSPI_ERASE_LEN_LEN_4KB);
}

int hal_exflash_erase_block(uintptr_t start_addr, size_t num_blocks)
{
    return erase_common(start_addr, num_blocks, QSPI_ERASE_LEN_LEN_64KB);
}

int hal_exflash_copy_sector(uintptr_t src_addr, uintptr_t dest_addr, size_t data_size)
{
    int ret = -1;
    if ((src_addr % sFLASH_PAGESIZE) ||
        (dest_addr % sFLASH_PAGESIZE) ||
        !(IS_WORD_ALIGNED(data_size)))
    {
        goto hal_exflash_copy_sector_done;
    }

    // erase sectors
    uint16_t sector_num = CEIL_DIV(data_size, sFLASH_PAGESIZE);
    if (hal_exflash_erase_sector(dest_addr, sector_num))
    {
        goto hal_exflash_copy_sector_done;
    }

    // memory copy
    uint8_t data_buf[FLASH_OPERATION_TEMP_BLOCK_SIZE] __attribute__((aligned(4)));
    unsigned index = 0;
    uint16_t copy_len;

    for (index = 0; index < data_size; index += copy_len)
    {
        copy_len = MIN((data_size - index), sizeof(data_buf));
        if (hal_exflash_read(src_addr + index, data_buf, copy_len))
        {
            goto hal_exflash_copy_sector_done;
        }

        if (hal_exflash_write(dest_addr + index, data_buf, copy_len))
        {
            goto hal_exflash_copy_sector_done;
        }
    }

    ret = 0;

hal_exflash_copy_sector_done:
    hal_exflash_unlock();
    return ret;
}
