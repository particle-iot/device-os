#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "flash_mal.h"
#include "flash_hal.h"
#include "flash_acquire.h"

#ifdef SOFTDEVICE_PRESENT
#include "nrf_fstorage_sd.h"
#else
#include "nrf_fstorage_nvmc.h"
#endif /* SOFTDEVICE_PRESENT */


#define CEIL_DIV(A, B)              (((A) + (B) - 1) / (B))
#define ADDR_ALIGN_WORD(addr)       ((addr) & 0xFFFFFFFC)


typedef enum {
    FS_OP_STATE_IDLE,
    FS_OP_STATE_BUSY,
    FS_OP_STATE_ERROR
} fs_op_state_t;

static volatile fs_op_state_t       fs_op_state = FS_OP_STATE_IDLE;

static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);
nrf_fstorage_t m_fs =
{
    .evt_handler = fstorage_evt_handler,
    .start_addr  = 0x1000,
    .end_addr    = 0x100000
};


static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    if (p_evt->result != NRF_SUCCESS)
    {
        LOG_DEBUG(ERROR, "*** Flash %s FAILED! (0x%x): addr=%p, len=0x%x bytes",
                      (p_evt->id == NRF_FSTORAGE_EVT_WRITE_RESULT) ? "write" : "erase",
                      p_evt->result, p_evt->addr, p_evt->len);
        *(fs_op_state_t *)p_evt->p_param = FS_OP_STATE_ERROR;
    }

    if (p_evt->p_param)
    {
        LOG_DEBUG(INFO, "fs_op_busy = false!");
        *(fs_op_state_t *)p_evt->p_param = FS_OP_STATE_IDLE;
    }
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

int hal_flash_write(uint32_t addr, const uint8_t * data_buf, uint32_t data_size)
{
    __flash_acquire();

    int ret = 0;
    uint32_t ret_code;
    uint32_t index = 0;

    uint32_t copy_size;
    uint32_t copy_data;
    uint8_t *copy_data_buf = (uint8_t *)&copy_data;

    if (fs_op_state != FS_OP_STATE_IDLE)
    {
        ret = -1;
        goto hal_flash_write_done;
    }

    // write head part
    copy_size = 4 - (addr & 0x03);
    copy_data = *(uint32_t *)ADDR_ALIGN_WORD(addr);
    memcpy(&copy_data_buf[addr & 0x03], data_buf, (data_size > copy_size) ? copy_size : data_size);
    if (copy_size)
    {
        LOG_DEBUG(TRACE, "write head, addr: 0x%x, head size: %d", ADDR_ALIGN_WORD(addr), copy_size);
        fs_op_state = FS_OP_STATE_BUSY; //should before calling nrf_fstorage_write
        ret_code = nrf_fstorage_write(&m_fs, ADDR_ALIGN_WORD(addr), copy_data_buf, 4,  (fs_op_state_t *)&fs_op_state);
        if (ret_code)
        {
            fs_op_state = FS_OP_STATE_IDLE;
            ret = -2;
            goto hal_flash_write_done;
        }
        while (fs_op_state == FS_OP_STATE_BUSY);
        if (fs_op_state == FS_OP_STATE_ERROR)
        {
            fs_op_state = FS_OP_STATE_IDLE;
            ret = -3;
            goto hal_flash_write_done;
        }
    }

    index += copy_size;
    if (index >= data_size)
    {
        ret = 0;
        goto hal_flash_write_done;
    }

    // write middle part
    if (data_size - index > 4)
    {
        LOG_DEBUG(TRACE, "write middle, addr: 0x%x, size: %d", ADDR_ALIGN_WORD(addr) + 4, data_size - index);
        uint32_t offset = 0;
        do {
            memcpy(copy_data_buf, &data_buf[index], 4);
            fs_op_state = FS_OP_STATE_BUSY;
            ret_code = nrf_fstorage_write(&m_fs, ADDR_ALIGN_WORD(addr) + 4 + offset, copy_data_buf, 4, (fs_op_state_t *)&fs_op_state);
            if (ret_code)
            {
                fs_op_state = FS_OP_STATE_IDLE;
                ret = -4;
                goto hal_flash_write_done;
            }
            while (fs_op_state == FS_OP_STATE_BUSY);
            if (fs_op_state == FS_OP_STATE_ERROR)
            {
                fs_op_state = FS_OP_STATE_IDLE;
                ret = -5;
                goto hal_flash_write_done;
            }

            index += 4;
            offset += 4;
        } while (data_size - index > 4);
    }

    // write tail part
    copy_size = data_size - index;
    copy_data = 0xFFFFFFFF;
    memcpy(copy_data_buf, &data_buf[index], copy_size);

    if (copy_size)
    {
        LOG_DEBUG(TRACE, "write tail, addr: 0x%x, tail size: %d", addr + data_size - copy_size, copy_size);
        fs_op_state = FS_OP_STATE_BUSY; //should before calling nrf_fstorage_write
        ret_code = nrf_fstorage_write(&m_fs, addr + data_size - copy_size, copy_data_buf, 4,  (fs_op_state_t *)&fs_op_state);
        if (ret_code)
        {
            fs_op_state = FS_OP_STATE_IDLE;
            ret = -6;
            goto hal_flash_write_done;
        }
        while (fs_op_state == FS_OP_STATE_BUSY);
        if (fs_op_state == FS_OP_STATE_ERROR)
        {
            fs_op_state = FS_OP_STATE_IDLE;
            ret = -7;
            goto hal_flash_write_done;
        }
    }

hal_flash_write_done:
    __flash_release();
    return ret;
}

int hal_flash_erase_sector(uint32_t addr, uint32_t num_sectors)
{
    __flash_acquire();

    int ret = 0;
    uint32_t ret_code = 0;

    LOG_DEBUG(TRACE, "nrf_fstorage_erase(addr=0x%p, len=%d pages)", addr, num_sectors);

    if (fs_op_state != FS_OP_STATE_IDLE)
    {
        ret = -1;
        goto hal_flash_erase_sector_done;
    }

    addr = (addr / INTERNAL_FLASH_PAGE_SIZE) * INTERNAL_FLASH_PAGE_SIZE; // Address must be aligned to a page boundary.
    fs_op_state = FS_OP_STATE_BUSY; //should before calling nrf_fstorage_erase 
    ret_code = nrf_fstorage_erase(&m_fs, addr, num_sectors,  (fs_op_state_t *)&fs_op_state);
    if (ret_code)
    {
        fs_op_state = FS_OP_STATE_IDLE;
        ret = -2;
        goto hal_flash_erase_sector_done;
    }
    while (fs_op_state == FS_OP_STATE_BUSY);
    if (fs_op_state == FS_OP_STATE_ERROR)
    {
        fs_op_state = FS_OP_STATE_IDLE;
        ret = -3;
        goto hal_flash_erase_sector_done;
    }

hal_flash_erase_sector_done:
    __flash_release();
    return ret;
}

int hal_flash_read(uint32_t addr, uint8_t * data_buf, uint32_t data_size)
{
    uint8_t *dest_data = (uint8_t *)(addr);
    memcpy(data_buf, dest_data, data_size);

    return 0;
}

int hal_flash_copy_sector(uint32_t src_addr, uint32_t dest_addr, uint32_t data_size)
{
    __flash_acquire();
    int ret = 0;

    #define MAX_COPY_LENGTH 256
    if ((src_addr % INTERNAL_FLASH_PAGE_SIZE) ||
        (dest_addr % INTERNAL_FLASH_PAGE_SIZE) ||
        (data_size & 0x3))
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

    // memory copy
    uint8_t data_buf[MAX_COPY_LENGTH];
    uint32_t index = 0;
    uint16_t copy_len = 0;

    for (index = 0; index < data_size; index += copy_len)
    {
        copy_len = (data_size - index) >= MAX_COPY_LENGTH ? MAX_COPY_LENGTH : (data_size - index);
        if (hal_flash_read(src_addr + index, data_buf, copy_len))
        {
            ret = -3;
            goto hal_flash_copy_sector_done;
        }

        if (hal_flash_write(dest_addr + index, data_buf, copy_len))
        {
            ret = -4;
            goto hal_flash_copy_sector_done;
        }
    }

hal_flash_copy_sector_done:
    return ret;
}
