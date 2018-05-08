#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "flash_hal.h"
#include "sdk_common.h"
#include "nrfx_config.h"
#include "nrf_log.h"

#ifdef SOFTDEVICE_PRESENT
#include "nrf_fstorage_sd.h"
#else
#include "nrf_fstorage_nvmc.h"
#endif


#define CEIL_DIV(A, B)              (((A) + (B) - 1) / (B))
#define ADDR_ALIGN_WORD(addr)       (addr & 0xFFFFFFFC)


static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);


nrf_fstorage_t m_fs =
{
    .evt_handler = fstorage_evt_handler,
    .start_addr  = 0x1000,
    .end_addr    = 0x100000
};

static uint32_t m_flash_operations_pending;


static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    if (NRF_LOG_ENABLED && (m_flash_operations_pending > 0))
    {
        m_flash_operations_pending--;
    }

    if (p_evt->result == NRF_SUCCESS)
    {
        NRF_LOG_INFO("*** Flash %s success: addr=%p, pending %d",
                      (p_evt->id == NRF_FSTORAGE_EVT_WRITE_RESULT) ? "write" : "erase",
                      p_evt->addr, m_flash_operations_pending);
    }
    else
    {
        NRF_LOG_INFO("*** Flash %s failed (0x%x): addr=%p, len=0x%x bytes, pending %d",
                      (p_evt->id == NRF_FSTORAGE_EVT_WRITE_RESULT) ? "write" : "erase",
                      p_evt->result, p_evt->addr, p_evt->len, m_flash_operations_pending);
    }

    if (p_evt->p_param)
    {
        //lint -save -e611 (Suspicious cast)
        ((flash_op_callback_t)(p_evt->p_param))((void*)p_evt->p_src);
        //lint -restore
    }
}

int hal_flash_init(void)
{
    uint32_t ret_code = nrf_fstorage_init(&m_fs, &nrf_fstorage_nvmc, NULL);
    NRF_LOG_INFO("m_fs, start: 0x%x ~ 0x%x", m_fs.start_addr, m_fs.end_addr);
    NRF_LOG_INFO("Initializing nrf_fstorage_nvmc backend.code: %d", ret_code);

    return (ret_code == NRF_SUCCESS) ? 0 : -1;
}

int hal_flash_write(uint32_t addr, const uint8_t * data_buf, uint32_t data_size)
{
    uint32_t ret_code;
    uint16_t index = 0;

    uint16_t copy_size;
    uint32_t copy_data;
    uint8_t *copy_data_buf = (uint8_t *)&copy_data;

    // write head part
    copy_size = 4 - (addr & 0x03);
    copy_data = *(uint32_t *)ADDR_ALIGN_WORD(addr);
    memcpy(&copy_data_buf[addr & 0x03], data_buf, (data_size > copy_size) ? copy_size : data_size);
    if (copy_size)
    {
        LOG_DEBUG("write head, addr: 0x%x, head size: %d", ADDR_ALIGN_WORD(addr), copy_size);
        ret_code = nrf_fstorage_write(&m_fs, ADDR_ALIGN_WORD(addr), copy_data_buf, 4, NULL);
        if (ret_code)
        {
            LOG_ERROR("ret_code: %d", ret_code);
            return -1;
        }
    }

    index += copy_size;
    if (index >= data_size)
    {
        return 0;
    }

    // write middle part
    if (data_size - index > 4)
    {
        LOG_DEBUG("write middle, addr: 0x%x, size: %d", ADDR_ALIGN_WORD(addr) + 4, data_size - index);
        uint8_t offset = 0;
        do {
            memcpy(copy_data_buf, &data_buf[index], 4);
            ret_code = nrf_fstorage_write(&m_fs, ADDR_ALIGN_WORD(addr) + 4 + offset, copy_data_buf, 4, NULL);
            if (ret_code)
            {
                LOG_ERROR("ret_code: %d", ret_code);
                return -2;
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
        LOG_DEBUG("write tail, addr: 0x%x, tail size: %d", addr + data_size - copy_size, copy_size);
        ret_code = nrf_fstorage_write(&m_fs, addr + data_size - copy_size, copy_data_buf, 4, NULL);
        if (ret_code)
        {
            LOG_ERROR("ret_code: %d", ret_code);
            return -3;
        }
    }
    
    return 0;
}

int hal_flash_erase_sector(uint32_t addr, uint32_t num_sectors)
{
    uint32_t ret_code;

    NRF_LOG_DEBUG("nrf_fstorage_erase(addr=0x%p, len=%d pages), queue usage: %d",
                  addr, num_sectors, m_flash_operations_pending);

    //lint -save -e611 (Suspicious cast)
    ret_code = nrf_fstorage_erase(&m_fs, addr, num_sectors, NULL);
    //lint -restore

    if ((NRF_LOG_ENABLED) && (ret_code == NRF_SUCCESS))
    {
        m_flash_operations_pending++;
    }
    else
    {
        NRF_LOG_WARNING("nrf_fstorage_erase() failed with error 0x%x.", ret_code);
    }

    return (ret_code == NRF_SUCCESS) ? 0 : -1;
}

int hal_flash_read(uint32_t addr, uint8_t * data_buf, uint32_t data_size)
{
    uint8_t *dest_data = (uint8_t *)(addr);
    memcpy(data_buf, dest_data, data_size);

    return 0;
}

int hal_flash_copy_sector(uint32_t src_addr, uint32_t dest_addr, uint32_t data_size)
{
    #define MAX_COPY_LENGTH 256
    if ((src_addr % INTERNAL_FLASH_SECTOR_SIZE) ||
        (dest_addr % INTERNAL_FLASH_SECTOR_SIZE) ||
        (data_size & 0x3))
    {
        return -1;
    }

    // erase sectors
    uint16_t sector_num = CEIL_DIV(data_size, INTERNAL_FLASH_SECTOR_SIZE);
    if (hal_flash_erase_sector(dest_addr, sector_num))
    {
        return -2;
    }

    // memory copy
    uint8_t data_buf[MAX_COPY_LENGTH];
    uint16_t index = 0;
    uint16_t copy_len;

    for (int i = 0; i < data_size;)
    {
        copy_len = (data_size - i) >= MAX_COPY_LENGTH ? MAX_COPY_LENGTH : data_size - i;
        if (hal_flash_read(src_addr + index, data_buf, copy_len))
        {
            return -3;
        }

        if (hal_flash_write(dest_addr + index, data_buf, copy_len))
        {
            return -4;
        }

        index += copy_len;
    }

    return 0;
}
