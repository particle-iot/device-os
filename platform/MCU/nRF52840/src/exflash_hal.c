#include "exflash_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nrfx_qspi.h"
#include "nrf_delay.h"
#include "app_util_platform.h"
#include "app_error.h"
#include "hw_config.h"

#define QSPI_STD_CMD_WRSR       0x01
#define QSPI_STD_CMD_RSTEN      0x66
#define QSPI_STD_CMD_RST        0x99

#define CEIL_DIV(A, B)              (((A) + (B) - 1) / (B))
#define ADDR_ALIGN_WORD(addr)       ((addr) & 0xFFFFFFFC)

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

int hal_exflash_init(void)
{
    uint32_t err_code = NRF_SUCCESS;
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

    err_code = nrfx_qspi_init(&config, NULL, NULL);
    if (err_code)
    {
        return -1;
    }
    LOG_DEBUG(TRACE, "QSPI example started.");

    if (configure_memory())
    {
        return -1;
    }

    return 0;
}

int hal_exflash_write(uint32_t addr, const uint8_t * data_buf, uint32_t data_size)
{
    uint32_t ret_code;
    uint32_t index = 0;

    uint32_t copy_size;
    uint32_t copy_data;
    uint8_t *copy_data_buf = (uint8_t *)&copy_data;

    // write head part
    copy_size = 4 - (addr & 0x03);
    ret_code = nrfx_qspi_read(copy_data_buf, 4, ADDR_ALIGN_WORD(addr));
    if (ret_code)
    {
        return -1;
    }
    memcpy(&copy_data_buf[addr & 0x03], data_buf, (data_size > copy_size) ? copy_size : data_size);
    if (copy_size)
    {
        LOG_DEBUG(TRACE, "write head, addr: 0x%x, head size: %d", ADDR_ALIGN_WORD(addr), copy_size);
        ret_code = nrfx_qspi_write(copy_data_buf, 4, ADDR_ALIGN_WORD(addr));
        if (ret_code)
        {
            LOG_DEBUG(ERROR, "ret_code: %d", ret_code);
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
        LOG_DEBUG(TRACE, "write middle, addr: 0x%x, size: %d", ADDR_ALIGN_WORD(addr) + 4, data_size - index);
        uint32_t offset = 0;
        do {
            memcpy(copy_data_buf, &data_buf[index], 4);
            ret_code = nrfx_qspi_write(copy_data_buf, 4, ADDR_ALIGN_WORD(addr) + 4 + offset);
            if (ret_code)
            {
                LOG_DEBUG(ERROR, "ret_code: %d", ret_code);
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
        LOG_DEBUG(TRACE, "write tail, addr: 0x%x, tail size: %d", addr + data_size - copy_size, copy_size);
        ret_code = nrfx_qspi_write(copy_data_buf, 4, addr + data_size - copy_size);
        if (ret_code)
        {
            LOG_DEBUG(ERROR, "ret_code: %d", ret_code);
            return -3;
        }
    }

    return 0;
}

int hal_exflash_read(uint32_t addr, uint8_t * data_buf, uint32_t data_size)
{
    uint32_t ret_code;
    uint32_t index = 0;

    uint32_t copy_size;
    uint32_t copy_data;
    uint8_t *copy_data_buf = (uint8_t *)&copy_data;

    // read head part
    copy_size = 4 - (addr & 0x03);
    ret_code = nrfx_qspi_read(copy_data_buf, 4, ADDR_ALIGN_WORD(addr));
    if (ret_code)
    {
        return -1;
    }
    memcpy(data_buf, &copy_data_buf[addr & 0x03], (data_size > copy_size) ? copy_size : data_size);
    index += copy_size;
    if (index >= data_size)
    {
        return 0;
    }

    // read middle part
    if (data_size - index > 4)
    {
        LOG_DEBUG(TRACE, "read middle, addr: 0x%x, size: %d", ADDR_ALIGN_WORD(addr) + 4, data_size - index);
        uint32_t offset = 0;
        do {
            ret_code = nrfx_qspi_read(copy_data_buf, 4, ADDR_ALIGN_WORD(addr + 4 + offset));
            if (ret_code)
            {
                return -2;
            }
            memcpy(&data_buf[index], copy_data_buf, 4);
            index += 4;
            offset += 4;
        } while (data_size - index > 4);
    }

    // read tail part
    copy_size = data_size - index;
    copy_data = 0;
    ret_code = nrfx_qspi_read(copy_data_buf, 4, addr + data_size - copy_size);
    if (ret_code)
    {
        return -3;
    }
    memcpy(&data_buf[index], copy_data_buf, copy_size);

    return 0;
}

int hal_exflash_erase_sector(uint32_t start_addr, uint32_t num_sectors)
{
    uint32_t err_code = NRF_SUCCESS;

    for (int i = 0; i < num_sectors; i++)
    {
        err_code = nrfx_qspi_erase(NRF_QSPI_ERASE_LEN_4KB, start_addr);
        if (err_code)
        {
            return -1;
        }
    }

    LOG_DEBUG(TRACE, "Process of erasing first block start");

    return 0;
}

int hal_exflash_erase_block(uint32_t start_addr, uint32_t num_blocks)
{
    uint32_t err_code = NRF_SUCCESS;

    for (int i = 0; i < num_blocks; i++)
    {
        err_code = nrfx_qspi_erase(NRF_QSPI_ERASE_LEN_64KB, start_addr);
        if (err_code)
        {
            return -1;
        }
    }

    LOG_DEBUG(TRACE, "Process of erasing first block start");

    return 0;
}

int hal_exflash_copy_sector(uint32_t src_addr, uint32_t dest_addr, uint32_t data_size)
{
    #define MAX_COPY_LENGTH 256
    if ((src_addr % sFLASH_PAGESIZE) ||
        (dest_addr % sFLASH_PAGESIZE) ||
        (data_size & 0x3))
    {
        return -1;
    }

    // erase sectors
    uint16_t sector_num = CEIL_DIV(data_size, sFLASH_PAGESIZE);
    if (hal_exflash_erase_sector(dest_addr, sector_num))
    {
        return -2;
    }

    // memory copy
    uint8_t data_buf[MAX_COPY_LENGTH];
    uint32_t index = 0;
    uint16_t copy_len;

    for (index = 0; index < data_size; index += copy_len)
    {
        copy_len = (data_size - index) >= MAX_COPY_LENGTH ? MAX_COPY_LENGTH : (data_size - index);
        if (hal_exflash_read(src_addr + index, data_buf, copy_len))
        {
            return -3;
        }

        if (hal_exflash_write(dest_addr + index, data_buf, copy_len))
        {
            return -4;
        }
    }

    return 0;
}
