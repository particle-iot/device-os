/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "app_util_platform.h"
#include "app_error.h"
#include "hw_config.h"
#include "logging.h"
#include "flash_common.h"
#include "nrf_nvic.h"
#include "concurrent_hal.h"
#include "gpio_hal.h"
#include "system_error.h"

enum qspi_cmds_t {
    QSPI_STD_CMD_WRSR     = 0x01,
    QSPI_STD_CMD_WREN     = 0x06,
    QSPI_STD_CMD_RSTEN    = 0x66,
    QSPI_STD_CMD_RST      = 0x99,
    QSPI_MX25_CMD_ENSO    = 0xb1,
    QSPI_MX25_CMD_EXSO    = 0xc1,
    QSPI_MX25_CMD_WRSCUR  = 0x2f,
    QSPI_MX25_CMD_SLEEP   = 0xB9,
    QSPI_MX25_CMD_READ_ID = 0xAB,
    QSPI_STD_CMD_PGMERS_SUSPEND = 0xB0
};

static const size_t MX25_OTP_SECTOR_SIZE = 4096 / 8; /* 4Kb or 512B */

static exflash_state_t qspi_state = HAL_EXFLASH_STATE_DISABLED;

// Mitigations for nRF52840 anomaly 215
// [215] QSPI: Reading QSPI registers after XIP might halt CPU
// Conditions
// Init and start QSPI, use XIP, then write to or read any QSPI register with an offset above 0x600.
//
// Consequences
// CPU halts.
//
// Workaround
// Trigger QSPI TASKS_ACTIVATE after XIP is used before accessing any QSPI register with an offset above 0x600.
//
// Our testing showed that another side effect of this anomaly is not a lockup
// but a failure to perform any Custom Instructions that rely on CISTR* registers
// of QSPI peripheral, which are located above offset 0x600.
//
// The mitigation that we are implementing is rather simple. We need to ensure that no XIP
// access occurs while we are performing Custom Instructions and we need to apply the workaround mentioned
// in the anomaly description (QSPI TASKS_ACTIVATE).
// There is a performance penalty because we are disabling RTOS thread scheduling,
// however because we are only doing this for CINSTR, it should be minimal.

static void exflash_qspi_activate() {
    nrf_qspi_event_clear(NRF_QSPI, NRF_QSPI_EVENT_READY);
    nrf_qspi_task_trigger(NRF_QSPI, NRF_QSPI_TASK_ACTIVATE);
    while (!nrf_qspi_event_check(NRF_QSPI, NRF_QSPI_EVENT_READY));
}

static nrfx_err_t exflash_qspi_wait_completion() {
    nrfx_err_t err = NRFX_ERROR_INTERNAL;

    // Certain operations like block erasure, may leave the flash
    // in a weird state where we can't talk to it for substantial periods of time
    // (up to ~30ms, seen in practice). According to the datasheet, the erasure for example may take up to 200ms!
    // Ensure we have initialized comms with the external flash properly by running TASK_ACTIVATE
    // before applying workaround to decrease the time we are staying with RTOS scheduling disabled.
    exflash_qspi_activate();

    do {
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
        // Enter critical section
        os_thread_scheduling(false, NULL);

        // Run QSPI TASK_ACTIVATE task as a workaround for the anomaly 215
        exflash_qspi_activate();
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

        // This may block for up to 1ms (QSPI_DEF_WAIT_TIME_US * QSPI_DEF_WAIT_ATTEMPTS)
        err = nrfx_qspi_mem_busy_check();

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
        // Exit critical section
        os_thread_scheduling(true, NULL);
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

    } while (err == NRFX_ERROR_BUSY);

    return err;
}

static nrfx_err_t exflash_qspi_cinstr_xfer(nrf_qspi_cinstr_conf_t const* p_config, void const* p_tx_buffer, void* p_rx_buffer) {
    // Certain operations like block erasure, may leave the flash
    // in a weird state where we can't talk to it for substantial periods of time
    // (up to ~30ms, seen in practice). According to the datasheet, the erasure for example may take up to 200ms!
    // Ensure we have initialized comms with the external flash properly by running TASK_ACTIVATE
    // before applying workaround to decrease the time we are staying with RTOS scheduling disabled.
    exflash_qspi_activate();

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    // Enter critical section
    os_thread_scheduling(false, NULL);

    // Run QSPI TASK_ACTIVATE task as a workaround for the anomaly 215
    exflash_qspi_activate();
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

    // This may block for up to 1ms (QSPI_DEF_WAIT_TIME_US * QSPI_DEF_WAIT_ATTEMPTS)
    nrfx_err_t err = nrfx_qspi_cinstr_xfer(p_config, p_tx_buffer, p_rx_buffer);

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    // Exit critical section
    os_thread_scheduling(true, NULL);
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    return err;
}

static nrfx_err_t exflash_qspi_cinstr_quick_send(uint8_t opcode, nrf_qspi_cinstr_len_t length, void const* p_tx_buffer) {
    nrf_qspi_cinstr_conf_t config = NRFX_QSPI_DEFAULT_CINSTR(opcode, length);
    return exflash_qspi_cinstr_xfer(&config, p_tx_buffer, NULL);
}

static int configure_memory() {
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
    err_code = exflash_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);
    if (err_code) {
        return -1;
    }

    // Send reset command
    cinstr_cfg.opcode = QSPI_STD_CMD_RST;
    err_code = exflash_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);
    if (err_code) {
        return -2;
    }

    //Switch to qpsi mode
#if HAL_PLATFORM_FLASH_MX25R6435FZNIL0
    uint8_t temporary[3] = { 0x40, 0x00, 0x02 };
    cinstr_cfg.length = NRF_QSPI_CINSTR_LEN_4B;
    cinstr_cfg.opcode = QSPI_STD_CMD_WRSR;
    err_code = exflash_qspi_cinstr_xfer(&cinstr_cfg, temporary, NULL);
#elif HAL_PLATFORM_FLASH_MX25L3233F
    uint8_t temporary[1] = { 0x40 };
    cinstr_cfg.length = NRF_QSPI_CINSTR_LEN_2B;
    cinstr_cfg.opcode = QSPI_STD_CMD_WRSR;
    err_code = exflash_qspi_cinstr_xfer(&cinstr_cfg, temporary, NULL);
#else
#error "Unsupported platform for external flash"
#endif

    if (err_code) {
        return -3;
    }

    return 0;
}

static int perform_write(uintptr_t addr, const uint8_t* data, size_t size) {
    return nrfx_qspi_write(data, size, addr);
}

static int enter_secure_otp() {
    return exflash_qspi_cinstr_quick_send(QSPI_MX25_CMD_ENSO, 1, NULL);
}

static int exit_secure_otp() {
    return exflash_qspi_cinstr_quick_send(QSPI_MX25_CMD_EXSO, 1, NULL);
}

int hal_exflash_init(void) {
    int ret = 0;

    // PATCH: Initialize CS pin, external memory discharge
    nrf_gpio_cfg_output(QSPI_FLASH_CSN_PIN);
    nrf_gpio_pin_set(QSPI_FLASH_CSN_PIN);
    nrf_gpio_cfg_input(QSPI_FLASH_IO1_PIN, NRF_GPIO_PIN_PULLDOWN);

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
    if (ret) {
        if (ret == NRFX_ERROR_TIMEOUT) {
            // Could have timed out because the flash chip might be in an ongoing program/erase operation.
            // Suspend it.
            LOG_DEBUG(TRACE, "QSPI NRFX timeout error. Suspend pgm/ers op.");
            ret = hal_exflash_special_command(HAL_EXFLASH_COMMAND_NONE, HAL_EXFLASH_COMMAND_SUSPEND_PGMERS, NULL, NULL, 0);
            if (ret) {
                goto hal_exflash_init_done;
            }
        }
    }
    LOG_DEBUG(TRACE, "QSPI initialized.");
    qspi_state = HAL_EXFLASH_STATE_ENABLED;

    // Wake up external flash from deep power down mode
    ret = hal_exflash_special_command(HAL_EXFLASH_COMMAND_NONE, HAL_EXFLASH_COMMAND_WAKEUP, NULL, NULL, 0);
    if (ret) {
        goto hal_exflash_init_done;
    }

    ret = configure_memory();
    if (ret) {
        goto hal_exflash_init_done;
    }

hal_exflash_init_done:
    hal_exflash_unlock();
    return ret;
}

int hal_exflash_uninit(void) {
    hal_exflash_lock();

    nrfx_qspi_uninit();
    // PATCH: Initialize CS pin, external memory discharge
    nrf_gpio_cfg_output(QSPI_FLASH_CSN_PIN);
    nrf_gpio_pin_set(QSPI_FLASH_CSN_PIN);
    nrf_gpio_cfg_input(QSPI_FLASH_IO1_PIN, NRF_GPIO_PIN_PULLDOWN);
    // The nrfx_qspi driver doesn't clear pending IRQ
    sd_nvic_ClearPendingIRQ(QSPI_IRQn);

    qspi_state = HAL_EXFLASH_STATE_DISABLED;

    hal_exflash_unlock();

    return 0;
}

int hal_exflash_write(uintptr_t addr, const uint8_t* data_buf, size_t data_size) {
    hal_exflash_lock();
    int ret = hal_flash_common_write(addr, data_buf, data_size,
                                     &perform_write, &hal_flash_common_dummy_read);
    exflash_qspi_wait_completion();
    hal_exflash_unlock();
    return ret;
}

int hal_exflash_read(uintptr_t addr, uint8_t* data_buf, size_t data_size) {
    int ret = 0;
    hal_exflash_lock();
    {
        const uintptr_t src_aligned = ADDR_ALIGN_WORD(addr);
        uint8_t* dst_aligned = (uint8_t*)ADDR_ALIGN_WORD_RIGHT((uintptr_t)data_buf);
        const size_t unaligned_bytes = MIN(data_size, (size_t)(dst_aligned - data_buf));
        const size_t size_aligned = ADDR_ALIGN_WORD(data_size - unaligned_bytes);

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

    for (int i = 0; i < num_blocks; i++) {
        err_code = nrfx_qspi_erase(len, start_addr);
        if (err_code) {
            goto erase_common_done;
        }

        exflash_qspi_wait_completion();

        start_addr += block_length;
    }

    // LOG_DEBUG(TRACE, "Erased %lu %lukB blocks starting from %" PRIxPTR,
    //           num_blocks, block_length / 1024, start_addr);

erase_common_done:
    hal_exflash_unlock();
    return err_code;
}

int hal_exflash_erase_sector(uintptr_t start_addr, size_t num_sectors) {
    return erase_common(start_addr, num_sectors, QSPI_ERASE_LEN_LEN_4KB);
}

int hal_exflash_erase_block(uintptr_t start_addr, size_t num_blocks) {
    return erase_common(start_addr, num_blocks, QSPI_ERASE_LEN_LEN_64KB);
}

int hal_exflash_copy_sector(uintptr_t src_addr, uintptr_t dest_addr, size_t data_size) {
    hal_exflash_lock();
    int ret = -1;
    if ((src_addr % sFLASH_PAGESIZE) ||
        (dest_addr % sFLASH_PAGESIZE) ||
        !(IS_WORD_ALIGNED(data_size))) {
        goto hal_exflash_copy_sector_done;
    }

    // erase sectors
    uint16_t sector_num = CEIL_DIV(data_size, sFLASH_PAGESIZE);
    if (hal_exflash_erase_sector(dest_addr, sector_num)) {
        goto hal_exflash_copy_sector_done;
    }

    // memory copy
    uint8_t data_buf[FLASH_OPERATION_TEMP_BLOCK_SIZE] __attribute__((aligned(4)));
    unsigned index = 0;
    uint16_t copy_len;

    for (index = 0; index < data_size; index += copy_len) {
        copy_len = MIN((data_size - index), sizeof(data_buf));
        if (hal_exflash_read(src_addr + index, data_buf, copy_len)) {
            goto hal_exflash_copy_sector_done;
        }
        if (hal_exflash_write(dest_addr + index, data_buf, copy_len)) {
            goto hal_exflash_copy_sector_done;
        }
    }

    ret = 0;

hal_exflash_copy_sector_done:
    hal_exflash_unlock();
    return ret;
}

int hal_exflash_read_special(hal_exflash_special_sector_t sp, uintptr_t addr, uint8_t* data_buf, size_t data_size) {
    /* The only special sector we have on MX25L3233F is OTP */
    if (sp != HAL_EXFLASH_SPECIAL_SECTOR_OTP) {
        return -1;
    }

    /* Validate bounds */
    if (addr + data_size > MX25_OTP_SECTOR_SIZE) {
        return -2;
    }

    hal_exflash_lock();

    /* Enter Secure OTP mode */
    int ret = enter_secure_otp();
    if (ret) {
        ret = -3;
        goto hal_exflash_read_special_done;
    }

    ret = hal_exflash_read(addr, data_buf, data_size);

hal_exflash_read_special_done:
    /* Exit Secure OTP mode */
    exit_secure_otp();
    hal_exflash_unlock();
    return ret;
}

int hal_exflash_write_special(hal_exflash_special_sector_t sp, uintptr_t addr, const uint8_t* data_buf, size_t data_size) {
    /* The only special sector we have on MX25L3233F is OTP */
    if (sp != HAL_EXFLASH_SPECIAL_SECTOR_OTP) {
        return -1;
    }

    /* Validate bounds */
    if (addr + data_size > MX25_OTP_SECTOR_SIZE) {
        return -1;
    }

    hal_exflash_lock();

    /* Enter Secure OTP mode */
    int ret = enter_secure_otp();
    if (ret) {
        goto hal_exflash_write_special_done;
    }

    ret = hal_exflash_write(addr, data_buf, data_size);

hal_exflash_write_special_done:
    /* Exit Secure OTP mode */
    exit_secure_otp();
    hal_exflash_unlock();
    return ret;
}

int hal_exflash_erase_special(hal_exflash_special_sector_t sp, uintptr_t addr, size_t size) {
    /* We only support OTP sector and since it's One Time Programmable, we can't erase it */
    return -1;
}

int hal_exflash_special_command(hal_exflash_special_sector_t sp, hal_exflash_command_t cmd, const uint8_t* data, uint8_t* result, size_t size) {
    int ret = -1;

    hal_exflash_lock();
    /* General commands */
    if (sp == HAL_EXFLASH_SPECIAL_SECTOR_NONE) {
        if (cmd == HAL_EXFLASH_COMMAND_SLEEP) {
            const nrf_qspi_cinstr_conf_t cinstr_cfg = {
                .opcode    = QSPI_MX25_CMD_SLEEP,
                .length    = NRF_QSPI_CINSTR_LEN_1B,
                .io2_level = true,
                .io3_level = true,
                .wipwait   = true,
                .wren      = true
            };

            /* Send sleep command */
            ret = exflash_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);
        } else if (cmd == HAL_EXFLASH_COMMAND_WAKEUP) {
            const nrf_qspi_cinstr_conf_t cinstr_cfg = {
                .opcode    = QSPI_MX25_CMD_READ_ID,
                .length    = NRF_QSPI_CINSTR_LEN_1B,
                .io2_level = true,
                .io3_level = true,
                .wipwait   = true,
                .wren      = true
            };

            /* Wake up external flash from deep power down modeby sending read id command */
            ret = exflash_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);
        } else if (cmd == HAL_EXFLASH_COMMAND_SUSPEND_PGMERS) {
            const nrf_qspi_cinstr_conf_t cinstr_cfg = {
                .opcode    = QSPI_STD_CMD_PGMERS_SUSPEND,
                .length    = NRF_QSPI_CINSTR_LEN_1B,
                .io2_level = true,
                .io3_level = true,
                .wipwait   = true,
                .wren      = true
            };
            /* suspend an ongoing program or erase operation */
            ret = exflash_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);
        }
    } else if (sp == HAL_EXFLASH_SPECIAL_SECTOR_OTP) {
        /* OTP-specific commands */
        if (cmd == HAL_EXFLASH_COMMAND_LOCK_ENTIRE_OTP) {
            /* Secured OTP Indicator bit (4K-bit Secured OTP)
             * 1 = factory lock
             * This will block the whole OTP region.
             */
            uint8_t v = 0x01;
            ret = exflash_qspi_cinstr_quick_send(QSPI_MX25_CMD_WRSCUR, 2, &v);
        }
    }

    hal_exflash_unlock();
    return ret;
}

int hal_exflash_sleep(bool sleep, void* reserved) {
    hal_exflash_lock();
    if (sleep) {
        // Suspend external flash
        if (qspi_state != HAL_EXFLASH_STATE_ENABLED) {
            hal_exflash_unlock();
            return SYSTEM_ERROR_INVALID_STATE;
        }
        // Put external flash into sleep and disable QSPI peripheral
        if (hal_exflash_special_command(HAL_EXFLASH_SPECIAL_SECTOR_NONE, HAL_EXFLASH_COMMAND_SLEEP, NULL, NULL, 0)) {
            // It may fail as the flash chip might be in an ongoing program/erase operation.
            // Suspend it.
            int ret = hal_exflash_special_command(HAL_EXFLASH_COMMAND_NONE, HAL_EXFLASH_COMMAND_SUSPEND_PGMERS, NULL, NULL, 0);
            if (ret) {
                hal_exflash_unlock();
                return ret;
            }
        }
        hal_exflash_uninit();
        qspi_state = HAL_EXFLASH_STATE_SUSPENDED;
    } else {
        // Restore external flash
        if (qspi_state != HAL_EXFLASH_STATE_SUSPENDED) {
            hal_exflash_unlock();
            return SYSTEM_ERROR_INVALID_STATE;
        }
        int ret = hal_exflash_init();
        if (ret != SYSTEM_ERROR_NONE) {
            hal_exflash_unlock();
            return ret;
        }
    }
    hal_exflash_unlock();
    return SYSTEM_ERROR_NONE;
}
