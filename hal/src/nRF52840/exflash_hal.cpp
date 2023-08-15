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

#include "exflash_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <algorithm>
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
#include "nrf_system_error.h"
#include "check.h"
#include "check_nrf.h"
#include "exflash_hal_params.h"

#define NRF_QSPI_DEFAULT_CINSTR(opc, len)   \
{                                           \
    .opcode    = (opc),                     \
    .length    = (len),                     \
    .io2_level = true,                      \
    .io3_level = true,                      \
    .wipwait   = true,                      \
    .wren      = true                       \
}

#define NRF_QSPI_QUICK_CFG(cmd_writeoc, cmd_readocc, clock_frequenccy)  \
{                                                                       \
    .xip_offset  = NRFX_QSPI_CONFIG_XIP_OFFSET,                         \
    .pins = {                                                           \
        .sck_pin     = QSPI_FLASH_SCK_PIN,                              \
        .csn_pin     = QSPI_FLASH_CSN_PIN,                              \
        .io0_pin     = QSPI_FLASH_IO0_PIN,                              \
        .io1_pin     = QSPI_FLASH_IO1_PIN,                              \
        .io2_pin     = QSPI_FLASH_IO2_PIN,                              \
        .io3_pin     = QSPI_FLASH_IO3_PIN,                              \
    },                                                                  \
    .prot_if = {                                                        \
        .readoc     = cmd_readocc,                                      \
        .writeoc    = cmd_writeoc,                                      \
        .addrmode   = NRF_QSPI_ADDRMODE_24BIT,                          \
        .dpmconfig  = false,                                            \
    },                                                                  \
    .phy_if = {                                                         \
        .sck_delay  = 1,  /* 62.5ns */                                  \
        .dpmen      = false,                                            \
        .spi_mode   = NRF_QSPI_MODE_0,                                  \
        .sck_freq   = clock_frequenccy                                  \
    },                                                                  \
    .irq_priority   = (uint8_t)QSPI_FLASH_IRQ_PRIORITY                  \
}

namespace {

class FlashLock {
public:
    FlashLock() {
        hal_exflash_lock();
    }
    ~FlashLock() {
        hal_exflash_unlock();
    }
};

hal_exflash_state_t qspi_state = HAL_EXFLASH_STATE_DISABLED;
hal_qspi_flash_type_t flash_type = HAL_QSPI_FLASH_TYPE_UNKNOWN;

hal_qspi_flash_type_t all_flashes[] = {
        HAL_QSPI_FLASH_TYPE_MX25L3233F,
        HAL_QSPI_FLASH_TYPE_MX25R6435F,
        HAL_QSPI_FLASH_TYPE_GD25WQ64E
    };

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

void exflash_qspi_activate() {
    nrf_qspi_event_clear(NRF_QSPI, NRF_QSPI_EVENT_READY);
    nrf_qspi_task_trigger(NRF_QSPI, NRF_QSPI_TASK_ACTIVATE);
    while (!nrf_qspi_event_check(NRF_QSPI, NRF_QSPI_EVENT_READY));
}

int exflash_qspi_wait_completion() {
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
        os_thread_scheduling(false, nullptr);

        // Run QSPI TASK_ACTIVATE task as a workaround for the anomaly 215
        exflash_qspi_activate();
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

        // This may block for up to 1ms (QSPI_DEF_WAIT_TIME_US * QSPI_DEF_WAIT_ATTEMPTS)
        err = nrfx_qspi_mem_busy_check();

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
        // Exit critical section
        os_thread_scheduling(true, nullptr);
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

    } while (err == NRFX_ERROR_BUSY);

    return nrf_system_error(err);
}

int exflash_qspi_cinstr_xfer(nrf_qspi_cinstr_conf_t const* p_config, void const* p_tx_buffer, void* p_rx_buffer) {
    // Certain operations like block erasure, may leave the flash
    // in a weird state where we can't talk to it for substantial periods of time
    // (up to ~30ms, seen in practice). According to the datasheet, the erasure for example may take up to 200ms!
    // Ensure we have initialized comms with the external flash properly by running TASK_ACTIVATE
    // before applying workaround to decrease the time we are staying with RTOS scheduling disabled.
    exflash_qspi_activate();

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    // Enter critical section
    os_thread_scheduling(false, nullptr);

    // Run QSPI TASK_ACTIVATE task as a workaround for the anomaly 215
    exflash_qspi_activate();
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

    // This may block for up to 1ms (QSPI_DEF_WAIT_TIME_US * QSPI_DEF_WAIT_ATTEMPTS)
    nrfx_err_t err = nrfx_qspi_cinstr_xfer(p_config, p_tx_buffer, p_rx_buffer);

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    // Exit critical section
    os_thread_scheduling(true, nullptr);
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    return nrf_system_error(err);
}

int exflash_qspi_cinstr_quick_send(uint8_t opcode, nrf_qspi_cinstr_len_t length, void const* p_tx_buffer) {
    nrf_qspi_cinstr_conf_t config = NRFX_QSPI_DEFAULT_CINSTR(opcode, length);
    return exflash_qspi_cinstr_xfer(&config, p_tx_buffer, nullptr);
}

int perform_write(uintptr_t addr, const uint8_t* data, size_t size) {
    return nrf_system_error(nrfx_qspi_write(data, size, addr));
}

constexpr nrf_qspi_cinstr_len_t get_nrf_cinstr_length(size_t length) {
    nrf_qspi_cinstr_len_t cinstrLen = NRF_QSPI_CINSTR_LEN_1B;
    switch (length) {
        case 1: cinstrLen = NRF_QSPI_CINSTR_LEN_1B; break;
        case 2: cinstrLen = NRF_QSPI_CINSTR_LEN_2B; break;
        case 3: cinstrLen = NRF_QSPI_CINSTR_LEN_3B; break;
        case 4: cinstrLen = NRF_QSPI_CINSTR_LEN_4B; break;
        case 5: cinstrLen = NRF_QSPI_CINSTR_LEN_5B; break;
        case 6: cinstrLen = NRF_QSPI_CINSTR_LEN_6B; break;
        case 7: cinstrLen = NRF_QSPI_CINSTR_LEN_7B; break;
        case 8: cinstrLen = NRF_QSPI_CINSTR_LEN_8B; break;
        case 9: cinstrLen = NRF_QSPI_CINSTR_LEN_9B; break;
        default: break;
    }
    return cinstrLen;
}

constexpr nrf_qspi_writeoc_t get_nrf_write_opcode(hal_qspi_flash_cmd_t write_cmd) {
    nrf_qspi_writeoc_t nrf_write = NRF_QSPI_WRITEOC_PP;
    switch (write_cmd) {
        case HAL_QSPI_CMD_STD_WRITE_PP4O:  nrf_write = NRF_QSPI_WRITEOC_PP4O; break;
        case HAL_QSPI_CMD_STD_WRITE_PP4IO: nrf_write = NRF_QSPI_WRITEOC_PP4IO; break;
        default: break;
    }
    return nrf_write;
}

constexpr nrf_qspi_readoc_t get_nrf_read_opcode(hal_qspi_flash_cmd_t read_cmd) {
    nrf_qspi_readoc_t nrf_read = NRF_QSPI_READOC_FASTREAD;
    switch (read_cmd) {
        case HAL_QSPI_CMD_STD_READ_4IO:  nrf_read = NRF_QSPI_READOC_READ4IO; break;
        default: break;
    }
    return nrf_read;
}

int mx25_enter_secure_otp() {
    return exflash_qspi_cinstr_quick_send(HAL_QSPI_CMD_MX25_ENSO, get_nrf_cinstr_length(1), nullptr);
}

int mx25_exit_secure_otp() {
    return exflash_qspi_cinstr_quick_send(HAL_QSPI_CMD_MX25_EXSO, get_nrf_cinstr_length(1), nullptr);
}

int mx25r64_switch_to_qspi() {
    nrf_qspi_cinstr_conf_t cinstr_cfg = NRF_QSPI_DEFAULT_CINSTR(HAL_QSPI_CMD_STD_WRSR, get_nrf_cinstr_length(4));
    // Status register, configuration register 1, configuration register 2 values
    uint8_t config_values[3] = { 0x40, 0x00, 0x02 };
    return exflash_qspi_cinstr_xfer(&cinstr_cfg, config_values, nullptr);
}

int mx25l32_switch_to_qspi() {
    nrf_qspi_cinstr_conf_t cinstr_cfg = NRF_QSPI_DEFAULT_CINSTR(HAL_QSPI_CMD_STD_WRSR, get_nrf_cinstr_length(2));
    uint8_t status_register[1] = { 0x40 };
    return exflash_qspi_cinstr_xfer(&cinstr_cfg, status_register, nullptr);
}

int gd25_switch_to_qspi() {
    nrf_qspi_cinstr_conf_t cinstr_cfg = NRF_QSPI_DEFAULT_CINSTR(HAL_QSPI_CMD_STD_WRSR, get_nrf_cinstr_length(2));
    uint8_t status_registers[3] = {0x00, 0x02, 0x00};

    CHECK(exflash_qspi_cinstr_xfer(&cinstr_cfg, &status_registers[0], nullptr));
    
    cinstr_cfg.opcode = HAL_QSPI_CMD_GD25_WRSR2;
    CHECK(exflash_qspi_cinstr_xfer(&cinstr_cfg, &status_registers[1], nullptr));

    cinstr_cfg.opcode = HAL_QSPI_CMD_GD25_WRSR3;
    CHECK(exflash_qspi_cinstr_xfer(&cinstr_cfg, &status_registers[2], nullptr));

    return 0;
}

int gd25_secure_register_read(uint8_t security_registers_index, uint32_t security_registers_addr, void* buf, size_t size) {
    CHECK_TRUE(security_registers_index < HAL_QSPI_FLASH_OTP_SECTOR_COUNT_GD25, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(security_registers_addr + size <= HAL_QSPI_FLASH_OTP_SIZE_GD25, SYSTEM_ERROR_INVALID_ARGUMENT);

    nrf_qspi_cinstr_conf_t readSecurityRegister = NRF_QSPI_DEFAULT_CINSTR(HAL_QSPI_CMD_GD25_SEC_READ, get_nrf_cinstr_length(1));

    // 3 bytes of address data + 1 byte of dummy data
    uint8_t addrBuf[4] = {}; 
    uint32_t address = ((security_registers_index + 1) << 12) + security_registers_addr;
    addrBuf[0] = (uint8_t)((address>>16) & 0xFF);
    addrBuf[1] = (uint8_t)((address>>8) & 0xFF);
    addrBuf[2] = (uint8_t)((address>>0) & 0xFF);
    addrBuf[3] = 0x00;

    // start long frame transfer with "Read Security Register" opcode
    nrfx_err_t ret = nrfx_qspi_lfm_start(&readSecurityRegister);
    CHECK_NRF_RETURN(ret, nrf_system_error(ret));
    nrf_delay_us(10); // need a bit delay to switch between normal flash and security register
    // Send address + dummy bytes, do not finalize long frame transfer
    ret = nrfx_qspi_lfm_xfer(addrBuf, nullptr, sizeof(addrBuf), false);
    CHECK_NRF_RETURN(ret, nrf_system_error(ret));
    // Read security register data, finalize long frame transfer
    ret = nrfx_qspi_lfm_xfer(nullptr, buf, size, true);
    CHECK_NRF_RETURN(ret, nrf_system_error(ret));

    return 0;
}

int gd25_secure_register_write(uint8_t security_registers_index, uint32_t security_registers_addr, void* buf, size_t size) {
    CHECK_TRUE(security_registers_index < HAL_QSPI_FLASH_OTP_SECTOR_COUNT_GD25, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(security_registers_addr + size <= HAL_QSPI_FLASH_OTP_SIZE_GD25, SYSTEM_ERROR_INVALID_ARGUMENT);

    nrf_qspi_cinstr_conf_t writeSecurityRegister = NRF_QSPI_DEFAULT_CINSTR(HAL_QSPI_CMD_GD25_SEC_PROGRAM, get_nrf_cinstr_length(1));

    uint32_t address = 0;
    uint8_t addrBuf[3] = {};
    address = ((security_registers_index + 1) << 12) + security_registers_addr;
    addrBuf[0] = (uint8_t)((address>>16) & 0xFF);
    addrBuf[1] = (uint8_t)((address>>8) & 0xFF);
    addrBuf[2] = (uint8_t)((address>>0) & 0xFF);

    nrfx_err_t ret = nrfx_qspi_lfm_start(&writeSecurityRegister);
    CHECK_NRF_RETURN(ret, nrf_system_error(ret));
    nrf_delay_us(10); // need a bit delay to switch between normal flash and security register
    // Send address bytes, do not finalize long frame transfer
    ret = nrfx_qspi_lfm_xfer(addrBuf, nullptr, sizeof(addrBuf), false);
    CHECK_NRF_RETURN(ret, nrf_system_error(ret));
    // Send data to write, finalize long frame transfer
    ret = nrfx_qspi_lfm_xfer((const void *)buf, nullptr, size, true);
    CHECK_NRF_RETURN(ret, nrf_system_error(ret));

    return 0;
}

int gd25_security_register_operation(hal_exflash_gd25_security_register_read_write operation, uintptr_t addr, uint8_t* data_buf, size_t data_size) {
    // Determine which OTP Register to start operating from
    size_t otp_page = addr / HAL_QSPI_FLASH_OTP_SECTOR_SIZE_GD25;
    size_t data_to_process = data_size;
    size_t data_processed = 0;
    size_t data_to_process_this_operation = 0;
    int ret = 0;

    while (data_to_process && !ret) {
        // Determine the offset of the current OTP register to start reading/writing from
        size_t register_offset = (addr + data_processed) % HAL_QSPI_FLASH_OTP_SECTOR_SIZE_GD25;
        data_to_process_this_operation = data_to_process;

        // If the data remaining to read/write extends into the next register, truncate the read/write amount to fit into this register
        if (register_offset + data_to_process_this_operation > HAL_QSPI_FLASH_OTP_SECTOR_SIZE_GD25){
            data_to_process_this_operation = HAL_QSPI_FLASH_OTP_SECTOR_SIZE_GD25 - register_offset;
        }

        ret = operation(otp_page, register_offset, &data_buf[data_processed], data_to_process_this_operation);

        // Increment/decrement counter variables, increment to the next OTP register
        data_to_process -= data_to_process_this_operation;
        data_processed += data_to_process_this_operation;
        otp_page++;
    }

    return ret;
}

int erase_common(uintptr_t start_addr, size_t num_blocks, nrf_qspi_erase_len_t len) {
    FlashLock lk;

    int err_code = NRF_SUCCESS;

    const size_t block_length = len == NRF_QSPI_ERASE_LEN_4KB ? 4096 : 64 * 1024;

    /* Round down to the nearest block */
    start_addr = ((start_addr / block_length) * block_length);

    for (size_t i = 0; i < num_blocks; i++) {
        err_code = nrfx_qspi_erase(len, start_addr);
        CHECK_NRF_RETURN(err_code, nrf_system_error(err_code));

        exflash_qspi_wait_completion();

        start_addr += block_length;
    }

    // LOG_DEBUG(TRACE, "Erased %lu %lukB blocks starting from %" PRIxPTR,
    //           num_blocks, block_length / 1024, start_addr);

    return 0;
}

} // Anonymous

int hal_exflash_init(void) {
    // PATCH: Initialize CS pin, external memory discharge
    nrf_gpio_cfg_output(QSPI_FLASH_CSN_PIN);
    nrf_gpio_pin_set(QSPI_FLASH_CSN_PIN);
    nrf_gpio_cfg_input(QSPI_FLASH_IO1_PIN, NRF_GPIO_PIN_PULLDOWN);

    FlashLock lk;

    // Use single data line SPI to read ID
    nrfx_qspi_config_t default_config = NRF_QSPI_QUICK_CFG(NRF_QSPI_WRITEOC_PP, NRF_QSPI_READOC_FASTREAD, NRF_QSPI_FREQ_32MDIV2);
    int ret = nrfx_qspi_init(&default_config, nullptr, nullptr);
    if (ret == NRFX_ERROR_TIMEOUT) {
        // Could have timed out because the flash chip might be in an ongoing program/erase operation or Deep Power-Down mode.
        // Suspend it or wake it up.
        LOG_DEBUG(TRACE, "QSPI NRFX timeout error. Suspend pgm/ers op or wake it up.");

        // Try all the supported suspend command
        for (auto flash: all_flashes) {
            flash_type = flash;
            hal_exflash_special_command(HAL_EXFLASH_SPECIAL_SECTOR_NONE, HAL_EXFLASH_COMMAND_SUSPEND_PGMERS, nullptr, nullptr, 0);
        }
    }

    // Determine chip type / wake up from potential sleep
    uint8_t chip_id[3] = {};

    // Temporarily set the flash type to make use of hal_exflash_special_command()
    flash_type = HAL_QSPI_DEFAULT_SPI_FLASH_TYPE;
    hal_exflash_special_command(HAL_EXFLASH_SPECIAL_SECTOR_NONE, HAL_EXFLASH_COMMAND_READID, nullptr, chip_id, sizeof(chip_id));
    flash_type = hal_exflash_get_type(chip_id);

    // If the chip is powered down, the READID command still returns a success result, but the chip_id is still NULL. 
    // If this happens, try to wake up the chip and READID again
    if (flash_type == HAL_QSPI_FLASH_TYPE_UNKNOWN) {
        for (auto flash: all_flashes) {
            flash_type = flash;
            hal_exflash_special_command(HAL_EXFLASH_SPECIAL_SECTOR_NONE, HAL_EXFLASH_COMMAND_WAKEUP, nullptr, nullptr, 0);
        }
        hal_exflash_special_command(HAL_EXFLASH_SPECIAL_SECTOR_NONE, HAL_EXFLASH_COMMAND_READID, nullptr, chip_id, sizeof(chip_id));
    }

    flash_type = hal_exflash_get_type(chip_id);
    // Read ID must succeed at this point
    CHECK_TRUE(flash_type != HAL_QSPI_FLASH_TYPE_UNKNOWN, SYSTEM_ERROR_NOT_FOUND);
    const hal_exflash_params_t* flash_params = hal_exflash_get_params(flash_type);

    // Re-initialize QSPI using corresponding flash parameters
    nrfx_qspi_uninit();
    nrfx_qspi_config_t new_config = NRF_QSPI_QUICK_CFG(get_nrf_write_opcode(flash_params->write_opcode), 
                                                       get_nrf_read_opcode(flash_params->read_opcode), 
                                                       NRF_QSPI_FREQ_32MDIV2);
    ret = nrfx_qspi_init(&new_config, nullptr, nullptr);
    CHECK_NRF_RETURN(ret, nrf_system_error(ret));
    qspi_state = HAL_EXFLASH_STATE_ENABLED;

    // Reset chip, put it into QSPI mode
    CHECK(hal_exflash_special_command(HAL_EXFLASH_SPECIAL_SECTOR_NONE, HAL_EXFLASH_COMMAND_RESET, nullptr, nullptr, 0));

    return SYSTEM_ERROR_NONE;
}

int hal_exflash_uninit(void) {
    FlashLock lk;

    nrfx_qspi_uninit();
    // PATCH: Initialize CS pin, external memory discharge
    nrf_gpio_cfg_output(QSPI_FLASH_CSN_PIN);
    nrf_gpio_pin_set(QSPI_FLASH_CSN_PIN);
    nrf_gpio_cfg_input(QSPI_FLASH_IO1_PIN, NRF_GPIO_PIN_PULLDOWN);
    // The nrfx_qspi driver doesn't clear pending IRQ
    sd_nvic_ClearPendingIRQ(QSPI_IRQn);

    qspi_state = HAL_EXFLASH_STATE_DISABLED;

    return SYSTEM_ERROR_NONE;
}

int hal_exflash_write(uintptr_t addr, const uint8_t* data_buf, size_t data_size) {
    FlashLock lk;

    int ret = hal_flash_common_write(addr, data_buf, data_size,
                                     &perform_write, &hal_flash_common_dummy_read);
    exflash_qspi_wait_completion();
    return ret;
}

int hal_exflash_read(uintptr_t addr, uint8_t* data_buf, size_t data_size) {
    FlashLock lk;

    int ret = 0;
    {
        const uintptr_t src_aligned = ADDR_ALIGN_WORD(addr);
        uint8_t* dst_aligned = (uint8_t*)ADDR_ALIGN_WORD_RIGHT((uintptr_t)data_buf);
        const size_t unaligned_bytes = std::min(data_size, (size_t)(dst_aligned - data_buf));
        const size_t size_aligned = ADDR_ALIGN_WORD(data_size - unaligned_bytes);

        if (size_aligned > 0) {
            /* Read-out the aligned portion into an aligned address in the buffer
             * with an adjusted size in multiples of 4.
             */
            ret = nrfx_qspi_read(dst_aligned, size_aligned, src_aligned);
            CHECK_NRF_RETURN(ret, nrf_system_error(ret));

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
        CHECK_NRF_RETURN(ret, nrf_system_error(ret));

        const unsigned offset_front = addr - src_aligned;

        memcpy(data_buf, tmpbuf + offset_front, data_size);
    }

    return 0;
}

int hal_exflash_erase_sector(uintptr_t start_addr, size_t num_sectors) {
    return erase_common(start_addr, num_sectors, NRF_QSPI_ERASE_LEN_4KB);
}

int hal_exflash_erase_block(uintptr_t start_addr, size_t num_blocks) {
    return erase_common(start_addr, num_blocks, NRF_QSPI_ERASE_LEN_64KB);
}

int hal_exflash_copy_sector(uintptr_t src_addr, uintptr_t dest_addr, size_t data_size) {
    FlashLock lk;

    uint8_t data_buf[FLASH_OPERATION_TEMP_BLOCK_SIZE] __attribute__((aligned(4)));
    unsigned index = 0;
    uint16_t copy_len;
    uint16_t sector_num;

    if ((src_addr % sFLASH_PAGESIZE) || (dest_addr % sFLASH_PAGESIZE) || !(IS_WORD_ALIGNED(data_size))) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    // erase sectors
    sector_num = CEIL_DIV(data_size, sFLASH_PAGESIZE);
    CHECK(hal_exflash_erase_sector(dest_addr, sector_num));

    // memory copy
    for (index = 0; index < data_size; index += copy_len) {
        copy_len = std::min((data_size - index), sizeof(data_buf));
        CHECK(hal_exflash_read(src_addr + index, data_buf, copy_len));
        CHECK(hal_exflash_write(dest_addr + index, data_buf, copy_len));
    }

    return 0;
}

int hal_exflash_read_special(hal_exflash_special_sector_t sp, uintptr_t addr, uint8_t* data_buf, size_t data_size) {
    const hal_exflash_params_t* flash_params = hal_exflash_get_params(flash_type);
    CHECK_TRUE(flash_params, SYSTEM_ERROR_NOT_FOUND);

    CHECK_TRUE(sp == HAL_EXFLASH_SPECIAL_SECTOR_OTP, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(addr + data_size <= flash_params->otp_size, SYSTEM_ERROR_OUT_OF_RANGE);
    CHECK_TRUE(data_buf != nullptr, SYSTEM_ERROR_INVALID_ARGUMENT);
    int ret = 0;

    FlashLock lk;

    if (flash_type == HAL_QSPI_FLASH_TYPE_GD25WQ64E) {
        ret = gd25_security_register_operation(gd25_secure_register_read, addr, data_buf, data_size);
    } else {
        ret = mx25_enter_secure_otp();
        if (!ret) {
            ret = hal_exflash_read(addr, data_buf, data_size);
        }
        mx25_exit_secure_otp(); 
    }

    return ret;
}

int hal_exflash_write_special(hal_exflash_special_sector_t sp, uintptr_t addr, const uint8_t* data_buf, size_t data_size) {
    const hal_exflash_params_t* flash_params = hal_exflash_get_params(flash_type);
    CHECK_TRUE(flash_params, SYSTEM_ERROR_NOT_FOUND);

    CHECK_TRUE(sp == HAL_EXFLASH_SPECIAL_SECTOR_OTP, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(addr + data_size <= flash_params->otp_size, SYSTEM_ERROR_OUT_OF_RANGE);
    CHECK_TRUE(data_buf != nullptr, SYSTEM_ERROR_INVALID_ARGUMENT);
    int ret = 0;

    FlashLock lk;

    if (flash_type == HAL_QSPI_FLASH_TYPE_GD25WQ64E) {
       ret = gd25_security_register_operation(gd25_secure_register_write, addr, (uint8_t *)data_buf, data_size);
    } else {
        ret = mx25_enter_secure_otp();
        if (!ret) {
            ret = hal_exflash_write(addr, data_buf, data_size);
        }
        mx25_exit_secure_otp();
    }

    return ret;
}

int hal_exflash_erase_special(hal_exflash_special_sector_t sp, uintptr_t addr, size_t size) {
    /* We only support OTP sector and since it's One Time Programmable, we can't erase it */
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_exflash_special_command(hal_exflash_special_sector_t sp, hal_exflash_command_t cmd, const uint8_t* data, uint8_t* result, size_t size) {
    FlashLock lk;

    /* General commands */
    if (sp == HAL_EXFLASH_SPECIAL_SECTOR_NONE) {
        nrf_qspi_cinstr_conf_t cinstr_cfg = {
            .opcode    = 0x00,
            .length    = NRF_QSPI_CINSTR_LEN_1B,
            .io2_level = true,
            .io3_level = true,
            .wipwait   = true,
            .wren      = true
        };

        const hal_exflash_params_t* flash_params = hal_exflash_get_params(flash_type);
        CHECK_TRUE(flash_params, SYSTEM_ERROR_NOT_FOUND);

        switch (cmd) {
            case HAL_EXFLASH_COMMAND_SLEEP:
                cinstr_cfg.opcode = HAL_QSPI_CMD_STD_SLEEP;
                break;
            case HAL_EXFLASH_COMMAND_WAKEUP:
                if (flash_type == HAL_QSPI_FLASH_TYPE_GD25WQ64E) {
                    cinstr_cfg.opcode = HAL_QSPI_CMD_GD25_WAKE;
                    cinstr_cfg.length = NRF_QSPI_CINSTR_LEN_1B;
                } else {
                    /* Wake up external flash from deep power down mode by sending read id command */
                    cinstr_cfg.opcode = HAL_QSPI_CMD_STD_READ_ID;
                    cinstr_cfg.length = NRF_QSPI_CINSTR_LEN_4B;
                }
                break;
            case HAL_EXFLASH_COMMAND_READID:
                cinstr_cfg.opcode = HAL_QSPI_CMD_STD_READ_ID;
                cinstr_cfg.length = NRF_QSPI_CINSTR_LEN_4B;
                break;
            case HAL_EXFLASH_COMMAND_SUSPEND_PGMERS:
                /* suspend an ongoing program or erase operation */
                cinstr_cfg.opcode = flash_params->suspend_opcode;
                break;
            case HAL_EXFLASH_COMMAND_RESET:
                cinstr_cfg.opcode = HAL_QSPI_CMD_STD_RSTEN;
                break;
            case HAL_EXFLASH_COMMAND_GET_SIZE: {
                const hal_exflash_params_t* flash_params = hal_exflash_get_params(flash_type);

                CHECK_TRUE(result != nullptr, SYSTEM_ERROR_INVALID_ARGUMENT);
                CHECK_TRUE(size >= sizeof(flash_params->size), SYSTEM_ERROR_INVALID_ARGUMENT);
                memcpy(result, &flash_params->size, sizeof(flash_params->size));
                return 0;
            }
            default:
                return SYSTEM_ERROR_NOT_SUPPORTED;
        }

        // If we are given a buffer for result data, ensure it is large enough, less the 1 byte opcode
        if (result != nullptr) {
            CHECK_TRUE(size >= ((size_t)cinstr_cfg.length)-1, SYSTEM_ERROR_INVALID_ARGUMENT);
        }

        // Execute the command.
        CHECK(exflash_qspi_cinstr_xfer(&cinstr_cfg, nullptr, result));

        if (cmd == HAL_EXFLASH_COMMAND_SUSPEND_PGMERS || cmd == HAL_EXFLASH_COMMAND_WAKEUP) {
            nrf_delay_us(20); // Need delay to ensure the flash is woken up.
        }

        // For the reset procedure, we need to execute the reset command going forward.
        if (cmd == HAL_EXFLASH_COMMAND_RESET) {
            cinstr_cfg.opcode = HAL_QSPI_CMD_STD_RST;
            CHECK(exflash_qspi_cinstr_xfer(&cinstr_cfg, nullptr, nullptr));
            switch (flash_type) {
                case HAL_QSPI_FLASH_TYPE_MX25L3233F: {
                    CHECK(mx25l32_switch_to_qspi());
                    break;
                };
                case HAL_QSPI_FLASH_TYPE_MX25R6435F: {
                    CHECK(mx25r64_switch_to_qspi());
                    break;
                };
                case HAL_QSPI_FLASH_TYPE_GD25WQ64E: {
                    CHECK(gd25_switch_to_qspi());
                    break;
                };
                default:
                    break;
            }
        }
    } else if (sp == HAL_EXFLASH_SPECIAL_SECTOR_OTP) {
        // OTP-specific commands
        switch (cmd) {
            case HAL_EXFLASH_COMMAND_LOCK_ENTIRE_OTP: {
                /* Secured OTP Indicator bit (4K-bit Secured OTP)
                 * 1 = factory lock
                 * This will block the whole OTP region.
                 */
                uint8_t v = 0x01;
                if (flash_type == HAL_QSPI_FLASH_TYPE_MX25L3233F || flash_type == HAL_QSPI_FLASH_TYPE_MX25R6435F) {
                    CHECK(exflash_qspi_cinstr_quick_send(HAL_QSPI_CMD_MX25_WRSCUR, get_nrf_cinstr_length(2), &v));
                } else {
                    // TODO: Implement for GD25 flash chip
                    return SYSTEM_ERROR_NOT_SUPPORTED;
                }
                break;
            };
            case HAL_EXFLASH_COMMAND_GET_OTP_SIZE: {
                const hal_exflash_params_t* flash_params = hal_exflash_get_params(flash_type);

                CHECK_TRUE(result != nullptr, SYSTEM_ERROR_INVALID_ARGUMENT);
                CHECK_TRUE(size >= sizeof(flash_params->otp_size), SYSTEM_ERROR_INVALID_ARGUMENT);

                memcpy(result, &flash_params->otp_size, sizeof(flash_params->otp_size));
                break;
            };
            default:
                return SYSTEM_ERROR_NOT_SUPPORTED;
        }
    }

    return SYSTEM_ERROR_NONE;
}

int hal_exflash_sleep(bool sleep, void* reserved) {
    FlashLock lk;

    if (sleep) {
        // Suspend external flash
        if (qspi_state != HAL_EXFLASH_STATE_ENABLED) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        // Put external flash into Deep Power-down mode
        if (hal_exflash_special_command(HAL_EXFLASH_SPECIAL_SECTOR_NONE, HAL_EXFLASH_COMMAND_SLEEP, nullptr, nullptr, 0)) {
            // It may fail as the flash chip might be in an ongoing program/erase operation. Suspend it.
            hal_exflash_special_command(HAL_EXFLASH_SPECIAL_SECTOR_NONE, HAL_EXFLASH_COMMAND_SUSPEND_PGMERS, nullptr, nullptr, 0);
            // Abort the suspended write/erase operation.
            hal_exflash_special_command(HAL_EXFLASH_SPECIAL_SECTOR_NONE, HAL_EXFLASH_COMMAND_RESET, nullptr, nullptr, 0);
            // Try enter Deep Power-down mode again
            int ret = hal_exflash_special_command(HAL_EXFLASH_SPECIAL_SECTOR_NONE, HAL_EXFLASH_COMMAND_SLEEP, nullptr, nullptr, 0);
            if (ret) {
                // Give up entering Deep Power-down mode.
                return ret;
            }
        }
        // Disable QSPI peripheral
        hal_exflash_uninit();
        qspi_state = HAL_EXFLASH_STATE_SUSPENDED;
    } else {
        // Restore external flash
        if (qspi_state != HAL_EXFLASH_STATE_SUSPENDED) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        int ret = hal_exflash_init();
        if (ret != SYSTEM_ERROR_NONE) {
            return ret;
        }
    }
    return SYSTEM_ERROR_NONE;
}
