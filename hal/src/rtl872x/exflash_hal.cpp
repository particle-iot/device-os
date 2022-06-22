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
#include <sys/param.h>
#include "hw_config.h"
#include "logging.h"
#include "flash_common.h"
#include "concurrent_hal.h"
#include "system_error.h"
extern "C" {
#include "rtl8721d.h"
}
#include "atomic_section.h"
#include "service_debug.h"
#include "check.h"
#include "scope_guard.h"

// TODO: move it to header file
#define HAL_EXFLASH_OTP_MAGIC_NUMBER_ADDR   0x0
#define HAL_EXFLASH_OTP_MAGIC_NUMBER        0x9A7D5BC6

extern uintptr_t platform_system_part1_flash_start;
extern uintptr_t platform_flash_end;

#if HAL_PLATFORM_FLASH_MX25R6435FZNIL0
typedef enum {
    MXIC_FLASH_CMD_ENSO             = 0xB1,
    MXIC_FLASH_CMD_EXSO             = 0xC1,
    MXIC_FLASH_CMD_DP               = 0xB9,
    MXIC_FLASH_CMD_RDID             = 0x9F,
    MXIC_FLASH_CMD_PGM_ERS_SPD      = 0x75,
    MXIC_FLASH_CMD_PGM_ERS_RSM      = 0x7A,
    MXIC_FLASH_CMD_RSTEN            = 0x66,
    MXIC_FLASH_CMD_RST              = 0x99,
    MXIC_FLASH_CMD_WREN             = 0x06,
    MXIC_FLASH_CMD_WRSCUR           = 0x2F
} mxic_flash_cmd_t;
#else
#error "Unsupported external flash"
#endif

// Constructor, destructors and methods are executed from PSRAM.
class ExFlashLock {
public:
    ExFlashLock(bool threading = true)
            : locked_(false),
              threading_(threading) {
        lock();
    }

    ~ExFlashLock() {
        if (locked_) {
            unlock();
        }
    }

    ExFlashLock(ExFlashLock&& lock)
            : locked_(lock.locked_),
              threading_(lock.threading_) {
        lock.locked_ = false;
        lock.threading_ = true;
    }

    void lock() {
        hal_exflash_lock();
        if (!threading_) {
            // Prevents other threads from accessing the external flash via XIP
            os_thread_scheduling(false, nullptr);
        }
        locked_ = true;
    }

    void unlock() {
        if (!threading_) {
            os_thread_scheduling(true, nullptr);
        }
        hal_exflash_unlock();
        locked_ = false;
    }

    ExFlashLock(const ExFlashLock&) = delete;
    ExFlashLock& operator=(const ExFlashLock&) = delete;

private:
    bool locked_;
    bool threading_;
};

static bool is_block_erased(uintptr_t addr, size_t size);

__attribute__((section(".ram.text"), noinline))
static int perform_write(uintptr_t addr, const uint8_t* data, size_t size) {
    // XXX: No way of knowing whether the write operation succeeded or not
    for (size_t b = 0; b < size;) {
        size_t rem = MIN(8, (size - b));
        // XXX: do not use 12 byte writes, sometimes we get deadlocked
        // TxData256 doesn't seem to work
        FLASH_TxData12B(addr + b, (uint8_t)rem, (uint8_t*)data + b);
        b += rem;
    }
    return SYSTEM_ERROR_NONE;
}

int hal_exflash_init(void) {
    return SYSTEM_ERROR_NONE;
}

int hal_exflash_uninit(void) {
    return SYSTEM_ERROR_NONE;
}

__attribute__((section(".ram.text"), noinline))
int hal_exflash_write(uintptr_t addr, const uint8_t* data_buf, size_t data_size) {
    ExFlashLock lk;
    CHECK(hal_flash_common_write(addr, data_buf, data_size, &perform_write, &hal_flash_common_dummy_read));
    DCache_CleanInvalidate(SPI_FLASH_BASE + addr, data_size);
    return SYSTEM_ERROR_NONE;
}

__attribute__((section(".ram.text"), noinline))
int hal_exflash_read(uintptr_t addr, uint8_t* data_buf, size_t data_size) {
    ExFlashLock lk;
    addr += SPI_FLASH_BASE;
    memcpy(data_buf, (void*)addr, data_size);
    return SYSTEM_ERROR_NONE;
}

__attribute__((section(".ram.text"), noinline))
static bool is_block_erased(uintptr_t addr, size_t size) {
    uint8_t* ptr = (uint8_t*)(SPI_FLASH_BASE + addr);
    for (size_t i = 0; i < size; i++) {
        if (ptr[i] != 0xff) {
            return false;
        }
    }
    return true;
}

__attribute__((section(".ram.text"), noinline))
static int erase_common(uintptr_t start_addr, size_t num_blocks, int len) {
    ExFlashLock lk;

    const size_t block_length = len == EraseSector ? 4096 : 64 * 1024;

    /* Round down to the nearest block */
    start_addr = ((start_addr / block_length) * block_length);

    for (size_t i = 0; i < num_blocks; i++) {
        if (!is_block_erased(start_addr + i * block_length, block_length)) {
            FLASH_Erase(len, start_addr + i * block_length);
        }
    }

    // LOG_DEBUG(ERROR, "Erased %lu %lukB blocks starting from %" PRIxPTR,
    //           num_blocks, block_length / 1024, start_addr);
    DCache_CleanInvalidate(SPI_FLASH_BASE + start_addr, block_length * num_blocks);

    return SYSTEM_ERROR_NONE;
}

__attribute__((section(".ram.text"), noinline))
int hal_exflash_erase_sector(uintptr_t start_addr, size_t num_sectors) {
    return erase_common(start_addr, num_sectors, EraseSector);
}

__attribute__((section(".ram.text"), noinline))
int hal_exflash_erase_block(uintptr_t start_addr, size_t num_blocks) {
    return erase_common(start_addr, num_blocks, EraseBlock);
}

__attribute__((section(".ram.text"), noinline))
int hal_exflash_copy_sector(uintptr_t src_addr, uintptr_t dest_addr, size_t data_size) {
    ExFlashLock lk;

    unsigned index = 0;
    uint16_t sector_num = CEIL_DIV(data_size, sFLASH_PAGESIZE);

    if ((src_addr % sFLASH_PAGESIZE) ||
        (dest_addr % sFLASH_PAGESIZE) ||
        !(IS_WORD_ALIGNED(data_size))) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    // erase sectors
    CHECK(hal_exflash_erase_sector(dest_addr, sector_num));

    // memory copy
    uint8_t data_buf[FLASH_OPERATION_TEMP_BLOCK_SIZE] __attribute__((aligned(4)));
    uint16_t copy_len;

    for (index = 0; index < data_size; index += copy_len) {
        copy_len = MIN((data_size - index), sizeof(data_buf));
        CHECK(hal_exflash_read(src_addr + index, data_buf, copy_len));
        CHECK(hal_exflash_write(dest_addr + index, data_buf, copy_len));
    }

    return SYSTEM_ERROR_NONE;
}

__attribute__((section(".ram.text"), noinline))
static bool isSecureOtpMode(uint32_t normalContent) {
    uint32_t temp = 0;
    FLASH_RxData(0, (uint32_t)HAL_EXFLASH_OTP_MAGIC_NUMBER_ADDR, sizeof(temp), (uint8_t*)&temp);
    if (temp == HAL_EXFLASH_OTP_MAGIC_NUMBER) {
        return true;
    }
    // Read the first word at 0x00000000 and compare it with the word we previously read in normal context.
    FLASH_RxData(0, (uint32_t)0, sizeof(temp), (uint8_t*)&temp);
    if (temp != normalContent) {
        return true;
    }
    return false;
}

// We are safe to run the constructor/destructor, cos they are copied to PSRAM to run.
class ProhibitXip {
public:
    ProhibitXip()
            : mpuCfg_{},
              mpuEntry_(0) {
        mpuEntry_ = mpu_entry_alloc();
        SPARK_ASSERT(mpuEntry_ >= 0 && mpuEntry_ < MPU_MAX_REGION);
        mpuCfg_.region_base = (uintptr_t)&platform_system_part1_flash_start;
        mpuCfg_.region_size = (uintptr_t)&platform_flash_end - (uintptr_t)&platform_system_part1_flash_start; // System part1, OTA region, user part and filesystem
        mpuCfg_.xn = MPU_EXEC_NEVER;
        mpuCfg_.ap = MPU_PRIV_R;
        mpuCfg_.sh = MPU_NON_SHAREABLE;
        mpuCfg_.attr_idx = MPU_MEM_ATTR_IDX_NC;
        mpu_region_cfg(mpuEntry_, &mpuCfg_);
    }

    ~ProhibitXip() {
        mpu_entry_free(mpuEntry_);
    }
private:
    mpu_region_config mpuCfg_;
    uint32_t mpuEntry_;
};

__attribute__((section(".ram.text"), noinline))
int hal_exflash_read_special(hal_exflash_special_sector_t sp, uintptr_t addr, uint8_t* data_buf, size_t data_size) {
    ExFlashLock lk(false); // Stop thread scheduler

    CHECK_TRUE(sp == HAL_EXFLASH_SPECIAL_SECTOR_OTP, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(data_buf && data_size > 0, SYSTEM_ERROR_INVALID_ARGUMENT);

    // Read the first word at 0x00000000
    uint32_t normalContent = 0;
    FLASH_RxData(0, (uint32_t)0, sizeof(normalContent), (uint8_t*)&normalContent);
    {
        ProhibitXip lk;

        SCOPE_GUARD({
            // Just in case, even if it might have failed to enter the secure OTP mode.
            FLASH_TxCmd(MXIC_FLASH_CMD_EXSO, 0, nullptr);
            if (isSecureOtpMode(normalContent)) {
                SPARK_ASSERT(false);
            }
        });

        FLASH_TxCmd(MXIC_FLASH_CMD_ENSO, 0, nullptr);
        if (isSecureOtpMode(normalContent)) {
            FLASH_RxData(0, (uint32_t)addr, data_size, data_buf);
        } else {
            return SYSTEM_ERROR_INVALID_STATE;
        }
    }
    return SYSTEM_ERROR_NONE;
}

__attribute__((section(".ram.text"), noinline))
int hal_exflash_write_special(hal_exflash_special_sector_t sp, uintptr_t addr, const uint8_t* data_buf, size_t data_size) {
    ExFlashLock lk(false); // Stop thread scheduler

    CHECK_TRUE(sp == HAL_EXFLASH_SPECIAL_SECTOR_OTP, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(data_buf && data_size > 0, SYSTEM_ERROR_INVALID_ARGUMENT);

    // Read the first word at 0x00000000
    uint32_t normalContent = 0;
    FLASH_RxData(0, (uint32_t)0, sizeof(normalContent), (uint8_t*)&normalContent);
    {
        ProhibitXip lk;

        SCOPE_GUARD({
            // Just in case, even if it might have failed to enter the secure OTP mode.
            FLASH_TxCmd(MXIC_FLASH_CMD_EXSO, 0, nullptr);
            if (isSecureOtpMode(normalContent)) {
                SPARK_ASSERT(false);
            }
        });

        FLASH_TxCmd(MXIC_FLASH_CMD_ENSO, 0, nullptr);
        if (isSecureOtpMode(normalContent)) {
            CHECK(hal_flash_common_write(addr, data_buf, data_size, &perform_write, &hal_flash_common_dummy_read));
        } else {
            return SYSTEM_ERROR_INVALID_STATE;
        }
    }
    return SYSTEM_ERROR_NONE;
}

int hal_exflash_erase_special(hal_exflash_special_sector_t sp, uintptr_t addr, size_t size) {
    /* We only support OTP sector and since it's One Time Programmable, we can't erase it */
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int hal_exflash_special_command(hal_exflash_special_sector_t sp, hal_exflash_command_t cmd, const uint8_t* data, uint8_t* result, size_t size) {
    ExFlashLock lk;

    uint8_t byte;

    /* General commands */
    if (sp == HAL_EXFLASH_SPECIAL_SECTOR_NONE) {
        if (cmd == HAL_EXFLASH_COMMAND_SLEEP) {
            byte = MXIC_FLASH_CMD_DP;
        } else if (cmd == HAL_EXFLASH_COMMAND_WAKEUP) {
            byte = MXIC_FLASH_CMD_RDID;
        } else if (cmd == HAL_EXFLASH_COMMAND_SUSPEND_PGMERS) {
            byte = MXIC_FLASH_CMD_PGM_ERS_SPD;
        } else if (cmd == HAL_EXFLASH_COMMAND_RESET) {
            byte = MXIC_FLASH_CMD_RSTEN;
        } else {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
        // FIXME: The result of the instruction is not garanteed.
        FLASH_TxCmd(byte, 0, nullptr);
        // For the reset procedure, we need to execute the reset command going forward.
        if (cmd == HAL_EXFLASH_COMMAND_RESET) {
            byte = MXIC_FLASH_CMD_RST;
            // FIXME: The result of the instruction is not garanteed.
            FLASH_TxCmd(byte, 0, nullptr);
        }
    } else if (sp == HAL_EXFLASH_SPECIAL_SECTOR_OTP) {
        if (cmd == HAL_EXFLASH_COMMAND_LOCK_ENTIRE_OTP) {
            byte = MXIC_FLASH_CMD_WREN;
            // FIXME: The result of the instruction is not garanteed.
            FLASH_TxCmd(byte, 0, nullptr);
            byte = MXIC_FLASH_CMD_WRSCUR;
            // FIXME: The result of the instruction is not garanteed.
            FLASH_TxCmd(byte, 0, nullptr);
        } else {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
    }

    return SYSTEM_ERROR_NONE;
}

int hal_exflash_sleep(bool sleep, void* reserved) {
    // Note: HAL sleep driver uses SDK API to suspend external flash.
    return SYSTEM_ERROR_NONE;
}
