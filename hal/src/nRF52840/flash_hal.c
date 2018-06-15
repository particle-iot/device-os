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

#ifdef SOFTDEVICE_PRESENT
#include "nrf_fstorage_sd.h"
#else
#include "nrf_fstorage_nvmc.h"
#endif /* SOFTDEVICE_PRESENT */

typedef enum {
    FS_OP_STATE_IDLE,
    FS_OP_STATE_BUSY,
    FS_OP_STATE_ERROR
} fs_op_state_t;

typedef void (*flash_op_callback_t)(void* p_buf);

static volatile fs_op_state_t       fs_op_state = FS_OP_STATE_IDLE;

static void fstorage_evt_handler(nrf_fstorage_evt_t* p_evt);

nrf_fstorage_t m_fs = {
    .evt_handler = fstorage_evt_handler,
    .start_addr  = 0x1000,
    .end_addr    = 0x100000
};


static void fstorage_evt_handler(nrf_fstorage_evt_t* p_evt)
{
    if (p_evt->result != NRF_SUCCESS) {
        LOG_DEBUG(ERROR, "*** Flash %s FAILED! (0x%x): addr=%p, len=0x%x bytes",
                  (p_evt->id == NRF_FSTORAGE_EVT_WRITE_RESULT) ? "write" : "erase",
                   p_evt->result, p_evt->addr, p_evt->len);
        *(fs_op_state_t *)p_evt->p_param = FS_OP_STATE_ERROR;
    } else {
        *(fs_op_state_t *)p_evt->p_param = FS_OP_STATE_IDLE;
    }
}

static int fstorage_perform_write(uintptr_t addr, const uint8_t* buf, size_t sz)
{
    int ret = -1;

    if (fs_op_state != FS_OP_STATE_IDLE) {
        return ret;
    }

    fs_op_state = FS_OP_STATE_BUSY;
    if (nrf_fstorage_write(&m_fs, addr, buf, sz, (fs_op_state_t *)&fs_op_state) == NRF_SUCCESS) {
        while (fs_op_state == FS_OP_STATE_BUSY) {
            /* Wait */
            /* TODO: use a semaphore in order not to busy-loop here? */
        }
        if (fs_op_state == FS_OP_STATE_IDLE) {
            ret = 0;
        }
    }

    fs_op_state = FS_OP_STATE_IDLE;
    return ret;
}

int hal_flash_init(void)
{
    __flash_acquire();
#ifndef SOFTDEVICE_PRESENT
    uint32_t ret_code = nrf_fstorage_init(&m_fs, &nrf_fstorage_nvmc, NULL);
    LOG_DEBUG(TRACE, "m_fs, range: 0x%x ~ 0x%x, erase size: %d", m_fs.start_addr, m_fs.end_addr, m_fs.p_flash_info->erase_unit);
    LOG_DEBUG(TRACE, "init nrf_fstorage_nvmc %s", ret_code == 0 ? "OK" : "FAILED!");
#else
    uint32_t ret_code = nrf_fstorage_init(&m_fs, &nrf_fstorage_sd, NULL);
    LOG_DEBUG(TRACE, "m_fs, range: 0x%x ~ 0x%x, erase size: %d", m_fs.start_addr, m_fs.end_addr, m_fs.p_flash_info->erase_unit);
    LOG_DEBUG(TRACE, "init nrf_fstorage_sd %s", ret_code == 0 ? "OK" : "FAILED!");
#endif /* SOFTDEVICE_PRESENT */

    __flash_release();
    return (ret_code == NRF_SUCCESS) ? 0 : -1;
}

int hal_flash_write(uintptr_t addr, const uint8_t* data_buf, size_t data_size)
{
    __flash_acquire();

    int ret = hal_flash_common_write(addr, data_buf, data_size,
                                     &fstorage_perform_write, &hal_flash_common_dummy_read);

    __flash_release();

    return ret;
}

int hal_flash_erase_sector(uintptr_t addr, size_t num_sectors)
{
    __flash_acquire();

    int ret = 0;
    uint32_t ret_code = 0;

    // LOG_DEBUG(TRACE, "nrf_fstorage_erase(addr=0x%p, len=%d pages)", addr, num_sectors);

    if (fs_op_state != FS_OP_STATE_IDLE)
    {
        ret = -1;
        goto hal_flash_erase_sector_done;
    }

    addr = (addr / INTERNAL_FLASH_PAGE_SIZE) * INTERNAL_FLASH_PAGE_SIZE; // Address must be aligned to a page boundary.
    fs_op_state = FS_OP_STATE_BUSY; //should before calling nrf_fstorage_erase
    ret_code = nrf_fstorage_erase(&m_fs, addr, num_sectors, (fs_op_state_t *)&fs_op_state);
    if (ret_code != NRF_SUCCESS) {
        fs_op_state = FS_OP_STATE_IDLE;
        ret = -2;
        goto hal_flash_erase_sector_done;
    }
    while (fs_op_state == FS_OP_STATE_BUSY) {
        /* Wait */
        /* TODO: use a semaphore in order not to busy-loop here? */
    }
    if (fs_op_state != FS_OP_STATE_IDLE) {
        fs_op_state = FS_OP_STATE_IDLE;
        ret = -3;
    }

hal_flash_erase_sector_done:
    __flash_release();
    return ret;
}

int hal_flash_read(uintptr_t addr, uint8_t* data_buf, size_t size)
{
    __flash_acquire();
    uint8_t* dest_data = (uint8_t*)(addr);
    memcpy(data_buf, dest_data, size);
    __flash_release();

    return 0;
}

int hal_flash_copy_sector(uintptr_t src_addr, uintptr_t dest_addr, size_t data_size)
{
    __flash_acquire();
    int ret = 0;

    if ((src_addr % INTERNAL_FLASH_PAGE_SIZE) ||
        (dest_addr % INTERNAL_FLASH_PAGE_SIZE) ||
        !IS_WORD_ALIGNED(data_size))
    {
        ret = -1;
        goto hal_flash_copy_sector_done;
    }

    // erase sectors
    uint16_t sector_num = CEIL_DIV(data_size, INTERNAL_FLASH_PAGE_SIZE);
    if (hal_flash_erase_sector(dest_addr, sector_num))
    {
        ret = -2;
        goto hal_flash_copy_sector_done;
    }

    if (hal_flash_write(dest_addr, (const uint8_t*)src_addr, data_size)) {
        ret = -3;
    }

hal_flash_copy_sector_done:
    __flash_release();
    return ret;
}
