#include "exflash_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nrfx_qspi.h"
#include "nrf_delay.h"
#include "app_util_platform.h"
#include "app_error.h"

#include "nrf_log.h"
#include "sdk_config.h"


#define QSPI_STD_CMD_WRSR       0x01
#define QSPI_STD_CMD_RSTEN      0x66
#define QSPI_STD_CMD_RST        0x99

#define CEIL_DIV(A, B)          (((A) + (B) - 1) / (B))

#define WAIT_FOR_PERIPH() do { \
        while (!m_finished) {} \
        m_finished = false;    \
    } while (0)


static volatile bool m_finished = false;


static void qspi_handler(nrfx_qspi_evt_t event, void * p_context)
{
    UNUSED_PARAMETER(event);
    UNUSED_PARAMETER(p_context);
    m_finished = true;
}

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
    nrfx_qspi_config_t config = NRFX_QSPI_DEFAULT_CONFIG;

    err_code = nrfx_qspi_init(&config, qspi_handler, NULL);
    if (err_code)
    {
        return -1;
    }
    NRF_LOG_INFO("QSPI example started.");

    if (configure_memory())
    {
        return -1;
    }

    return 0;
}

int hal_exflash_write(uint32_t addr, void const * data_buf, uint32_t data_size)
{
    uint32_t err_code = nrfx_qspi_write(data_buf, data_size, addr);
    if (err_code)
    {
        return -1;
    }

    WAIT_FOR_PERIPH();
    NRF_LOG_INFO("Process of writing data start");

    return 0;
}

int hal_exflash_erase_sector(uint32_t start_addr, uint32_t num_sectors)
{
    uint32_t err_code = NRF_SUCCESS;

    m_finished = false;
    for (int i = 0; i < num_sectors; i++)
    {
        err_code = nrfx_qspi_erase(NRF_QSPI_ERASE_LEN_4KB, start_addr);
        if (err_code)
        {
            return -1;
        }
        WAIT_FOR_PERIPH();
    }

    NRF_LOG_INFO("Process of erasing first block start");

    return 0;
}

int hal_exflash_erase_block(uint32_t start_addr, uint32_t num_blocks)
{
    uint32_t err_code = NRF_SUCCESS;

    m_finished = false;
    for (int i = 0; i < num_blocks; i++)
    {
        err_code = nrfx_qspi_erase(NRF_QSPI_ERASE_LEN_64KB, start_addr);
        if (err_code)
        {
            return -1;
        }
        WAIT_FOR_PERIPH();
    }

    NRF_LOG_INFO("Process of erasing first block start");

    return 0;
}

int hal_exflash_read(uint32_t addr, void * data_buf, uint32_t data_size)
{
    uint32_t err_code = nrfx_qspi_read(data_buf, data_size, addr);
    if (err_code)
    {
        return -1;
    }
    WAIT_FOR_PERIPH();
    NRF_LOG_INFO("Data read");

    return 0;
}

int hal_exflash_copy_sector(uint32_t src_addr, uint32_t dest_addr, uint32_t data_size)
{
    #define MAX_COPY_LENGTH 256
    if ((src_addr % EXFLASH_SECTOR_SIZE) ||
        (dest_addr % EXFLASH_SECTOR_SIZE) ||
        (data_size & 0x3))
    {
        return -1;
    }

    // erase sectors
    uint16_t sector_num = CEIL_DIV(data_size, EXFLASH_SECTOR_SIZE);
    if (hal_exflash_erase_sector(dest_addr, sector_num))
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
        if (hal_exflash_read(src_addr + index, data_buf, copy_len))
        {
            return -3;
        }

        if (hal_exflash_write(dest_addr + index, data_buf, copy_len))
        {
            return -4;
        }

        index += copy_len;
    }

    return 0;
}
