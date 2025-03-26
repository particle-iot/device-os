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
#include <FreeRTOS.h> // for portBYTE_ALIGNMENT

#include <algorithm>

// TODO: move it to header file
#define HAL_EXFLASH_OTP_MAGIC_NUMBER_ADDR   0x0
#define HAL_EXFLASH_OTP_MAGIC_NUMBER        0x9A7D5BC6

extern uintptr_t platform_system_part1_flash_start;
extern uintptr_t platform_flash_end;

typedef enum {
    MXIC_FLASH_CMD_WRSCUR           = 0x2F,
    MXIC_FLASH_CMD_ENSO             = 0xB1,
    MXIC_FLASH_CMD_EXSO             = 0xC1,
    MXIC_FLASH_CMD_4PP              = 0x38,

    GD_FLASH_CMD_ERS_SEC            = 0x44,
    GD_FLASH_CMD_PGM_SEC            = 0x42,
    GD_FLASH_CMD_RD_SEC             = 0x48,
    GD_FLASH_CMD_4PP                = 0x32,
    GD_FLASH_CMD_FPP                = 0xf2,

    GENERIC_FLASH_CMD_RST           = 0x99,
    GENERIC_FLASH_CMD_RSTEN         = 0x66,
    GENERIC_FLASH_CMD_PGM_ERS_SPD   = 0x75,
    GENERIC_FLASH_CMD_PGM_ERS_RSM   = 0x7A,
    GENERIC_FLASH_CMD_RDID          = 0x9F,
    GENERIC_FLASH_CMD_DP            = 0xB9,
    GENERIC_FLASH_CMD_WREN          = 0x06,
    GENERIC_FLASH_CMD_WRDI          = 0x04,
    GENERIC_FLASH_CMD_RDSR          = 0x05,
    GENERIC_FLASH_CMD_WRSR          = 0x01,
    GENERIC_FLASH_CMD_SE            = 0x20,
    GENERIC_FLASH_CMD_BE32K         = 0x52,
    GENERIC_FLASH_CMD_BE64K         = 0xd8,
    GENERIC_FLASH_CMD_PP            = 0x02
} hal_exflash_cmd_t;

#define HAL_QSPI_FLASH_OTP_SECTOR_COUNT_GD25        3
#define HAL_QSPI_FLASH_OTP_SECTOR_SIZE_GD25         1024
#define HAL_QSPI_FLASH_OTP_SIZE_GD25                (HAL_QSPI_FLASH_OTP_SECTOR_SIZE_GD25 * HAL_QSPI_FLASH_OTP_SECTOR_COUNT_GD25)

typedef int (*Gd25SecurityRegistersOperation)(uint8_t, uint32_t, uint8_t*, size_t);

void dcacheCleanInvalidateAligned(uintptr_t ptr, size_t size) {
    uintptr_t alignedPtr = ptr & ~(portBYTE_ALIGNMENT_MASK);
    uintptr_t end = ptr + size;
    size_t alignedSize = CEIL_DIV(end - alignedPtr, portBYTE_ALIGNMENT) * portBYTE_ALIGNMENT;
    DCache_CleanInvalidate(alignedPtr, alignedSize);
}

class RsipIfRequired {
public:
    RsipIfRequired(uint32_t address) {
        if ((HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SYS_EFUSE_SYSCFG3) & BIT_SYS_FLASH_ENCRYPT_EN) == 0) {
            return;
        }

        uint32_t km0_system_control = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_KM0_CTRL);
        if ((km0_system_control & BIT_LSYS_PLFM_FLASH_SCE) != 0) {
            return;
        }

        if (!isEncryptedRegion(address)) {
            return;
        }

        interruptState_ = HAL_disable_irq();

        HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_KM0_CTRL, (km0_system_control | BIT_LSYS_PLFM_FLASH_SCE));
        enabled_ = true;
    }

    ~RsipIfRequired() {
        if (enabled_) {
            uint32_t km0_system_control = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_KM0_CTRL);
            HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_KM0_CTRL, (km0_system_control & (~BIT_LSYS_PLFM_FLASH_SCE)));
            HAL_enable_irq(interruptState_);
        }
    }

private:
    bool isEncryptedRegion(uint32_t address) {
        uint8_t userEfuse0 = 0xFF;
        EFUSE_PMAP_READ8(0, USER_KEY_0_EFUSE_ADDRESS, &userEfuse0, L25EOUTVOLTAGE);
        bool part1_encryption_enabled = !(userEfuse0 & PART1_ENCRYPTED_BIT);

        if (address >= KM0_MBR_START_ADDRESS && address < (KM0_MBR_START_ADDRESS + KM0_MBR_IMAGE_SIZE) /* MBR */) {
            return true;
        } else if (part1_encryption_enabled && (address >= KM0_PART1_START_ADDRESS && address < (KM0_PART1_START_ADDRESS + KM0_PART1_IMAGE_SIZE)) /* part1 */) {
            return true;
        }

        return false;
    }

    bool enabled_ = false;
    int interruptState_ = 0;
};

#define RTK_SPIC_FIFO_SIZE                  64
#define RTK_SPIC_FIFO_HALF_SIZE             (RTK_SPIC_FIFO_SIZE / 2)

#define HAL_QSPI_FLASH_PAGE_SIZE            ((size_t)256)

#define RTK_SPIC_BIT_OFFSET_AUTO_LEN_DUM    ((uint32_t)0)
#define RTK_SPIC_BIT_WIDTH_AUTO_LEN_DUM     ((uint32_t)16)
#define RTK_SPIC_BIT_OFFSET_SR_TXE          ((uint32_t)5)
#define RTK_SPIC_BIT_OFFSET_SR_BUSY         ((uint32_t)0)

#define RTK_SPIC_DATA_WIDTH_MODE_SINGLE        (0)
#define RTK_SPIC_DATA_WIDTH_MODE_QUAD_I        (0x00080000)
#define RTK_SPIC_DATA_WIDTH_MODE_QUAD_II       (0x000a0000)
#define RTK_SPIC_DATA_WIDTH_MODE_DUAL_I        (0x00040000)
#define RTK_SPIC_DATA_WIDTH_MODE_DUAL_II       (0x00050000)

#define RTK_SPIC_READ_TUNING_DUMMY_CYCLE    0x2

#define RTK_SPIC_BIT_MASK(offset) ((uint32_t)((offset) == 32) ?                     \
    0x0 : (0x1 << (offset)))

#define RTK_SPIC_BIT_MASK_WIDTH(offset, bit_num) ((uint32_t)((offset) == 32) ?      \
    0xFFFFFFFF : (((1 << (bit_num)) - 1) << (offset)))

#define RTK_SPIC_BITS_SET_VAL(reg, offset, val, bit_num)                            \
    ((reg) = ((uint32_t)(reg) & (~RTK_SPIC_BIT_MASK_WIDTH(offset, bit_num))) |      \
    ((val << (offset)) & RTK_SPIC_BIT_MASK_WIDTH(offset, bit_num)))

#define RTK_SPIC_CMD_ADDR_FORMAT(cmd, addr) (((cmd) & 0x000000ff) |                 \
    ((addr & 0x000000ff) << 24) |                                                   \
    ((addr & 0x0000ff00) << 8) |                                                    \
    ((addr & 0x00ff0000) >> 8))

#define RTK_SPIC_BIT_GET_UNSHIFTED(reg, offset)                                     \
    ((uint32_t)((reg) & RTK_SPIC_BIT_MASK(offset)))

enum RtkSpicFlashDataWidth {
    RTK_SPIC_DATA_BYTE = 0,
    RTK_SPIC_DATA_HALF = 1,
    RTK_SPIC_DATA_WORD = 2
};

static void rtkSpiFlashSetDataReg(uint32_t index, uint32_t data, enum RtkSpicFlashDataWidth byteWidth) {
    if (index > 31) {
        return;
    } else if (byteWidth == RTK_SPIC_DATA_BYTE) {
        SPIC->dr[index].byte = (data & 0x000000ff);
    } else if (byteWidth == RTK_SPIC_DATA_HALF) {
        SPIC->dr[index].half = (data & 0x0000ffff);
    } else if (byteWidth == RTK_SPIC_DATA_WORD) {
        SPIC->dr[index].word = data;
    }
}

static void rtkSpiSetTxMode(uint32_t mode = RTK_SPIC_DATA_WIDTH_MODE_SINGLE) {
    SPIC->ctrlr0 = SPIC->ctrlr0 & 0xfffffcff;
    SPIC->ctrlr0 = (SPIC->ctrlr0 & 0xfff0ffff);
    SPIC->ctrlr0 = (SPIC->ctrlr0) | (mode);
}

static void rtkSpiFlashWaitBusy() {
    while (1) {
        if (RTK_SPIC_BIT_GET_UNSHIFTED(SPIC->sr, RTK_SPIC_BIT_OFFSET_SR_TXE)) {
            break;
        }
        if ((!RTK_SPIC_BIT_GET_UNSHIFTED(SPIC->sr, RTK_SPIC_BIT_OFFSET_SR_BUSY))) {
            break;
        }
    }
}

#ifdef ARM_CPU_CORTEX_M33
static void rtkSpiSetRxMode() {
    SPIC->ctrlr0 = (SPIC->ctrlr0 | 0x0300);
    SPIC->ctrlr0 = (SPIC->ctrlr0 & 0xfff0ffff);
}

static uint32_t rtkSpiFlashGetDataReg(uint32_t index, enum RtkSpicFlashDataWidth byteWidth) {
    uint32_t data;
    if (index > 31) {
        return 0;
    } else if (byteWidth == RTK_SPIC_DATA_BYTE) {
        data = SPIC->dr[index].byte & 0x000000ff;
    } else if (byteWidth == RTK_SPIC_DATA_HALF) {
        data = SPIC->dr[index].half & 0x0000ffff;
    } else if (byteWidth == RTK_SPIC_DATA_WORD) {
        data = SPIC->dr[index].word;
    } else {
        return 0;
    }
    return data;
}

static uint8_t rtkSpiFlashReadSR() {
    /* Disable SPI_FLASH */
    SPIC->ssienr = 0;
    /* Set Ctrlr1; 1 byte data frames */
    SPIC->ctrlr1 = 1;
    /* Set tuning dummy cycles */
    RTK_SPIC_BITS_SET_VAL(SPIC->auto_length, RTK_SPIC_BIT_OFFSET_AUTO_LEN_DUM, RTK_SPIC_READ_TUNING_DUMMY_CYCLE, RTK_SPIC_BIT_WIDTH_AUTO_LEN_DUM);
    /* set ctrlr0: RX_mode */
    rtkSpiSetRxMode();
    /* set flash_cmd: write cmd to fifo */
    rtkSpiFlashSetDataReg(0, GENERIC_FLASH_CMD_RDSR, RTK_SPIC_DATA_BYTE);
    /* Enable SPI_FLASH */
    SPIC->ssienr = 1;
    rtkSpiFlashWaitBusy();
    return rtkSpiFlashGetDataReg(0, RTK_SPIC_DATA_BYTE);
}
#endif // ARM_CPU_CORTEX_M33

static void rtkSpiFlashEnableCsWriteErase() {
    /* Enable SPI_FLASH */
    SPIC->ssienr = 1;
    rtkSpiFlashWaitBusy();
    /* Check flash is in write progress or not */
    for (;;) {
        uint8_t status = 0;
#ifdef ARM_CPU_CORTEX_M23
        // WORKAROUND: SPIC on KM0 reports invalid status register bits. ROM function does not
        FLASH_RxCmd(GENERIC_FLASH_CMD_RDSR, 1, &status);
#else 
        status = rtkSpiFlashReadSR();
#endif // ARM_CPU_CORTEX_M23

        if (!(status & 0x1)) {
            break;
        }
    }
}

static void rtkSpiFlashSendCmd(uint8_t cmd) {
    /* Disble SPI_FLASH */
    SPIC->ssienr = 0;
    /* set ctrlr0: TX mode */
    rtkSpiSetTxMode();
    /* set flash_cmd: wren to fifo */
    rtkSpiFlashSetDataReg(0, cmd, RTK_SPIC_DATA_BYTE);
    rtkSpiFlashEnableCsWriteErase();
}

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

static void rtkSpiFlashEnableCsRead(uint32_t len) {
    /* set receive data length */
    if (len <= 0x00010000) {
        SPIC->ctrlr1 = len;
    }
    /* Enable SPI_FLASH */
    SPIC->ssienr = 1;
    rtkSpiFlashWaitBusy();
}

static void rtkSpiFlashRxCmdAddr(uint8_t cmd, uint32_t addr, uint32_t dummyCycle) {
    /* Disable SPI_FLASH */
    SPIC->ssienr = 0;
    if (dummyCycle != 0) { // spi_flash_set_dummy_cycle(dev, dummy);
        uint32_t cycle = 0;
        /* if using fast_read baud_rate */
        if (((SPIC->ctrlr0) & 0x00100000)) {
            cycle = (SPIC->fbaudr);
        } else {
            cycle = (SPIC->baudr);
        }
        cycle = (cycle * dummyCycle * 2) + RTK_SPIC_READ_TUNING_DUMMY_CYCLE;
        if (cycle <= 0x10000) {
            RTK_SPIC_BITS_SET_VAL(SPIC->auto_length, RTK_SPIC_BIT_OFFSET_AUTO_LEN_DUM, cycle, RTK_SPIC_BIT_WIDTH_AUTO_LEN_DUM);
        }
    } else {
        /* Set tuning dummy cycles */
        RTK_SPIC_BITS_SET_VAL(SPIC->auto_length, RTK_SPIC_BIT_OFFSET_AUTO_LEN_DUM, RTK_SPIC_READ_TUNING_DUMMY_CYCLE, RTK_SPIC_BIT_WIDTH_AUTO_LEN_DUM);
    }

    /* set ctrlr0: RX mode, data_ch, addr_ch */
    rtkSpiSetRxMode();

    /* set flash cmd + addr and write to fifo */
    SPIC->dr[0].word = RTK_SPIC_CMD_ADDR_FORMAT(cmd, addr);
}

#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

static void rtkSpiFlashTxCmdAddr(uint8_t cmd, uint32_t addr, uint32_t mode = RTK_SPIC_DATA_WIDTH_MODE_SINGLE) {
    /* Disable SPI_FLASH */
    SPIC->ssienr = 0;
    /* set ctrlr0: TX mode, data_ch, addr_ch */
    rtkSpiSetTxMode(mode);
    /* set flash cmd + addr and write to fifo */
    SPIC->dr[0].word = RTK_SPIC_CMD_ADDR_FORMAT(cmd, addr);
}

class RtkSpicFlashConfigGuard {
public:
    RtkSpicFlashConfigGuard() {
        ctrl0_ = SPIC->ctrlr0;
        autoLen_ = SPIC->auto_length;
        addrLen_ = SPIC->addr_length;
        /* setting auto mode CS_H_WR_LEN, address length(3 byte) */
        SPIC->auto_length = ((SPIC->auto_length & 0xfffcffff) | 0x00030000);
        SPIC->addr_length = 3;
    }
    ~RtkSpicFlashConfigGuard() {
        SPIC->ssienr = 0;
        SPIC->ctrlr0 = ctrl0_;
        SPIC->auto_length = autoLen_;
        SPIC->addr_length = addrLen_;
    }

private:
    uint32_t ctrl0_;
    uint32_t autoLen_;
    uint32_t addrLen_;
};

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
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
        if (!threading_) {
            // Prevents other threads from accessing the external flash via XIP
            os_thread_scheduling(false, nullptr);
        }
#endif // MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
        locked_ = true;
    }

    void unlock() {
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
        if (!threading_) {
            os_thread_scheduling(true, nullptr);
        }
#endif // MODULE_FUNCTION == MOD_FUNC_BOOTLOADER
        hal_exflash_unlock();
        locked_ = false;
    }

    ExFlashLock(const ExFlashLock&) = delete;
    ExFlashLock& operator=(const ExFlashLock&) = delete;

private:
    bool locked_;
    bool threading_;
};

static int writeInBestModeWithinPageBoundaries(uint32_t addr, uint8_t* buf, size_t size) {
    uint32_t address = addr;
    uint8_t cmd = GENERIC_FLASH_CMD_PP;
    uint32_t mode = RTK_SPIC_DATA_WIDTH_MODE_SINGLE;

    if (flash_init_para.FLASH_cur_bitmode == SpicQuadBitMode) {
        // Quad mode enabled
        if (flash_init_para.FLASH_Id == FLASH_ID_GD) {
            cmd = GD_FLASH_CMD_4PP;
            mode = RTK_SPIC_DATA_WIDTH_MODE_QUAD_I;
        } else {
            cmd = MXIC_FLASH_CMD_4PP;
            mode = RTK_SPIC_DATA_WIDTH_MODE_QUAD_II;
        }
    } else if (flash_init_para.FLASH_cur_bitmode == SpicDualBitMode) {
        // Dual mode enabled
        // FIXME: gd25q128ewig doesn't support this looks like?
        // if (flash_init_para.FLASH_Id == FLASH_ID_GD) {
        //     cmd = GD_FLASH_CMD_FPP;
        //     mode = RTK_SPIC_DATA_WIDTH_MODE_DUAL_II;
        // }
    }

    size_t rem = addr % HAL_QSPI_FLASH_PAGE_SIZE;
    if (rem == 0) {
        // Can write the full page up to its size or less
        size = std::min(HAL_QSPI_FLASH_PAGE_SIZE, size);
    } else {
        // Can write only up to page boundary otherwise we'll wrap around
        size = std::min(HAL_QSPI_FLASH_PAGE_SIZE - rem, size);
    }

    size_t written = size;

    while (size > 0) {
        ExFlashLock lk(false);
        size_t writeLen = 0;
        {
            rtkSpiFlashSendCmd(GENERIC_FLASH_CMD_WREN);
            rtkSpiFlashTxCmdAddr(cmd, address, mode);
            if (size > (RTK_SPIC_FIFO_HALF_SIZE - 4)) {
                writeLen = RTK_SPIC_FIFO_HALF_SIZE - 4;
            } else {
                writeLen = size;
            }
            uint32_t data;
            uint32_t cnt = writeLen / 4;
            for (uint32_t i = 0; i < cnt; i++) {
                memcpy(&data, buf, 4);
                buf += 4;
                rtkSpiFlashSetDataReg(0, data, RTK_SPIC_DATA_WORD);
            }
            cnt = writeLen % 4;
            if (cnt > 0) {
                if (cnt >= 2) {
                    memcpy(&data, buf, 2);
                    buf += 2;
                    rtkSpiFlashSetDataReg(0, data, RTK_SPIC_DATA_HALF);
                    cnt -= 2;
                }
                if (cnt > 0) {
                    memcpy(&data, buf, 1);
                    buf += 1;
                    rtkSpiFlashSetDataReg(0, data, RTK_SPIC_DATA_BYTE);
                }
            }
        }
        rtkSpiFlashEnableCsWriteErase();
        size -= writeLen;
        address += writeLen;
    }

    return written;
}

static int writeInBestMode(uint32_t addr, uint8_t* buf, size_t size) {
    uint32_t address = addr;

    RtkSpicFlashConfigGuard guard;

    while (size > 0) {
        auto written = writeInBestModeWithinPageBoundaries(address, buf, size);
        address += written;
        buf += written;
        size -= written;
    }

    rtkSpiFlashSendCmd(GENERIC_FLASH_CMD_WRDI);
    DCache_Invalidate(addr + SPI_FLASH_BASE, size);

    return SYSTEM_ERROR_NONE;
}

// We are safe to run the constructor/destructor, cos they are copied to PSRAM to run.
class XipControl {
public:
    XipControl() {
        if (controlled_) {
            enableXip(true);
        }
    }

    ~XipControl() {
        if (controlled_) {
            enableXip(false);
        }
    }

    static int initXipOff() {
        enableXip(false);
        controlled_ = true;
        return 0;
    }

private:
    static void enableXip(bool enable) {
        (void)mpuEntry_;
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
        if (mpuEntry_ < 0) {
            // Allocate entry, but just once
            // This is safe and done under an exflash lock normally
            mpuEntry_ = mpu_entry_alloc();
        }
        SPARK_ASSERT(mpuEntry_ >= 0);

        mpu_region_config mpu_cfg = {};
        mpu_cfg.region_base = (uintptr_t)&platform_system_part1_flash_start;
        mpu_cfg.region_size = (uintptr_t)&platform_flash_end - (uintptr_t)&platform_system_part1_flash_start; // System part1, OTA region, user part and filesystem
        if (enable) {
            mpu_cfg.xn = MPU_EXEC_ALLOW;
            mpu_cfg.ap = MPU_UN_PRIV_RW;
            mpu_cfg.sh = MPU_NON_SHAREABLE;
            mpu_cfg.attr_idx = MPU_MEM_ATTR_IDX_WB_T_RWA;
        } else {
            mpu_cfg.xn = MPU_EXEC_NEVER;
            mpu_cfg.ap = MPU_PRIV_R;
            mpu_cfg.sh = MPU_NON_SHAREABLE;
            mpu_cfg.attr_idx = MPU_MEM_ATTR_IDX_NC;
        }
        mpu_region_cfg(mpuEntry_, &mpu_cfg);
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    }

private:
    static bool controlled_;
    static signed char mpuEntry_;
};
bool XipControl::controlled_ = false;
signed char XipControl::mpuEntry_ = -1;

static bool is_block_erased(uintptr_t addr, size_t size);

static int writeInBestMode(uint32_t addr, uint8_t* buf, size_t size);

__attribute__((section(".ram.text"), noinline))
static int perform_write(uintptr_t addr, const uint8_t* data, size_t size) {
    return writeInBestMode(addr, (uint8_t*)data, size);
}

int hal_exflash_init(void) {
    // ExFlashLock lk;
    // FLASH_ClockSwitch(BIT_SHIFT_FLASH_CLK_XTAL, _FALSE);
    return SYSTEM_ERROR_NONE;
}

int hal_exflash_uninit(void) {
    return SYSTEM_ERROR_NONE;
}

__attribute__((section(".ram.text"), noinline))
int hal_exflash_write(uintptr_t addr, const uint8_t* data_buf, size_t data_size) {
    ExFlashLock lk;
    CHECK(hal_flash_common_write(addr, data_buf, data_size, &perform_write, &hal_flash_common_dummy_read, 0));
    dcacheCleanInvalidateAligned(addr + SPI_FLASH_BASE, data_size);
    return SYSTEM_ERROR_NONE;
}

__attribute__((section(".ram.text"), noinline))
int hal_exflash_read(uintptr_t addr, uint8_t* data_buf, size_t data_size) {
    ExFlashLock lk;
    XipControl xiplk;
    addr += SPI_FLASH_BASE;

    RsipIfRequired rsip(addr); // FIXME: data_size as well?
    memcpy(data_buf, (void*)addr, data_size);
    dcacheCleanInvalidateAligned((uintptr_t)data_buf, data_size);
    return SYSTEM_ERROR_NONE;
}

__attribute__((section(".ram.text"), noinline))
static bool is_block_erased(uintptr_t addr, size_t size) {
    ExFlashLock lk;
    XipControl xiplk;
    uint8_t* ptr = (uint8_t*)(SPI_FLASH_BASE + addr);
    for (size_t i = 0; i < size; i++) {
        if (ptr[i] != 0xff) {
            return false;
        }
    }
    return true;
}

__attribute__((section(".ram.text"), noinline))
static int erase_common(uintptr_t address, hal_exflash_cmd_t cmd) {
    ExFlashLock lk(false);
    size_t size = 0;
    if (cmd == GENERIC_FLASH_CMD_SE) {
        size = 4 * 1024;
    } else if (cmd == GENERIC_FLASH_CMD_BE32K) {
        size = 32 * 1024;
    } else if (cmd == GENERIC_FLASH_CMD_BE64K) {
        size = 64 * 1024;
    } else {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    address = ((address / (size)) * (size));
    if (is_block_erased(address, (size))) {
        return size;
    }
    RtkSpicFlashConfigGuard guard;

    rtkSpiFlashSendCmd(GENERIC_FLASH_CMD_WREN);
    /* Disable SPI_FLASH */
    SPIC->ssienr = 0;
    rtkSpiSetTxMode();
    /* set flash cmd + addr and write to fifo */
    SPIC->dr[0].word = RTK_SPIC_CMD_ADDR_FORMAT(cmd, address);
    rtkSpiFlashEnableCsWriteErase();
    rtkSpiFlashSendCmd(GENERIC_FLASH_CMD_WRDI);
    return size;
}

__attribute__((section(".ram.text"), noinline))
int hal_exflash_erase_sector(uintptr_t start_addr, size_t num_sectors) {
    size_t sizeInBytes = num_sectors * 4096;
    uintptr_t addr = start_addr;
    while (sizeInBytes > 0) {
        hal_exflash_cmd_t cmd = GENERIC_FLASH_CMD_SE;
        if (addr % (64*1024) == 0 && sizeInBytes >= 64*1024) {
            cmd = GENERIC_FLASH_CMD_BE64K;
        } else if (addr % (32*1024) == 0 && sizeInBytes >= 32*1024) {
            cmd = GENERIC_FLASH_CMD_BE32K;
        }
        size_t size = CHECK(erase_common(addr, cmd));
        addr += size;
        sizeInBytes -= size;
    }
    dcacheCleanInvalidateAligned(SPI_FLASH_BASE + start_addr, num_sectors * 4096);
    return 0;
}

__attribute__((section(".ram.text"), noinline))
int hal_exflash_erase_block(uintptr_t start_addr, size_t num_blocks) {
    for (size_t i = 0; i < num_blocks; i++) {
        CHECK(erase_common(start_addr, GENERIC_FLASH_CMD_BE64K));
    }
    dcacheCleanInvalidateAligned(SPI_FLASH_BASE + start_addr, num_blocks * 64 * 4096);
    return 0;
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

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

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

#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

static int gd25ReadSecurityRegisters(uint8_t otpPageIdx, uint32_t addr, uint8_t* buf, size_t size) {
    CHECK_TRUE(otpPageIdx < HAL_QSPI_FLASH_OTP_SECTOR_COUNT_GD25, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(addr + size <= HAL_QSPI_FLASH_OTP_SIZE_GD25, SYSTEM_ERROR_INVALID_ARGUMENT);

    uint32_t address = ((otpPageIdx + 1) << 12) + addr;
    uint32_t dummyCycle = 8;
    uint8_t cmd = GD_FLASH_CMD_RD_SEC;

    RtkSpicFlashConfigGuard guard;

    while (size > 0) {
        rtkSpiFlashRxCmdAddr(cmd, address, dummyCycle);
        size_t readLen = 0;
        if (size > RTK_SPIC_FIFO_HALF_SIZE) {
            readLen = RTK_SPIC_FIFO_HALF_SIZE;
        } else {
            readLen = size;
        }
        rtkSpiFlashEnableCsRead(readLen);
        uint32_t data;
        uint32_t cnt = readLen / 4;
        for (uint32_t i = 0; i < cnt; i++) {
            data = rtkSpiFlashGetDataReg(0, RTK_SPIC_DATA_WORD);
            memcpy(buf, &data, 4);
            buf += 4;
        }
        cnt = readLen % 4;
        if (cnt > 0) {
            data = rtkSpiFlashGetDataReg(0, RTK_SPIC_DATA_WORD);
            memcpy(buf, &data, cnt);
            buf += cnt;
        }
        size -= readLen;
        address += readLen;
    }

    dcacheCleanInvalidateAligned((uintptr_t)buf, size);
    return SYSTEM_ERROR_NONE;
}

static int gd25WriteSecurityRegisters(uint8_t otpPageIdx, uint32_t addr, uint8_t* buf, size_t size) {
    CHECK_TRUE(otpPageIdx < HAL_QSPI_FLASH_OTP_SECTOR_COUNT_GD25, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(addr + size <= HAL_QSPI_FLASH_OTP_SIZE_GD25, SYSTEM_ERROR_INVALID_ARGUMENT);

    uint32_t address = ((otpPageIdx + 1) << 12) + addr;
    uint8_t cmd = GD_FLASH_CMD_PGM_SEC;

    RtkSpicFlashConfigGuard guard;

    while (size > 0) {
        rtkSpiFlashSendCmd(GENERIC_FLASH_CMD_WREN);
        rtkSpiFlashTxCmdAddr(cmd, address);
        size_t writeLen = 0;
        // rtkSpiFlashTxCmdAddr() has put 4 bytes into FIFO
        if (size > (RTK_SPIC_FIFO_HALF_SIZE - 4)) {
            writeLen = RTK_SPIC_FIFO_HALF_SIZE - 4;
        } else {
            writeLen = size;
        }
        uint32_t data;
        uint32_t cnt = writeLen / 4;
        for (uint32_t i = 0; i < cnt; i++) {
            memcpy(&data, buf, 4);
            buf += 4;
            rtkSpiFlashSetDataReg(0, data, RTK_SPIC_DATA_WORD);
        }
        cnt = writeLen % 4;
        if (cnt > 0) {
            if (cnt >= 2) {
                memcpy(&data, buf, 2);
                buf += 2;
                rtkSpiFlashSetDataReg(0, data, RTK_SPIC_DATA_HALF);
                cnt -= 2;
            }
            if (cnt > 0) {
                memcpy(&data, buf, 1);
                buf += 1;
                rtkSpiFlashSetDataReg(0, data, RTK_SPIC_DATA_BYTE);
            }
        }
        rtkSpiFlashEnableCsWriteErase();
        size -= writeLen;
        address += writeLen;
    }
    rtkSpiFlashSendCmd(GENERIC_FLASH_CMD_WRDI);

    return SYSTEM_ERROR_NONE;
}

static int gd25PerformSecurityRegistersOperation(Gd25SecurityRegistersOperation operation, uintptr_t addr, uint8_t* data_buf, size_t data_size) {
    ExFlashLock lk(false); // Stop thread scheduler

    // Determine which OTP Register to start operating from
    size_t otpPageIdx = addr / HAL_QSPI_FLASH_OTP_SECTOR_SIZE_GD25;
    size_t dataToProcessTotal = data_size;
    size_t dataProcessed = 0;
    size_t dataToProcess = 0;
    int ret = 0;

    while (dataToProcessTotal && !ret) {
        // Determine the offset of the current OTP register to start reading/writing from
        size_t register_offset = (addr + dataProcessed) % HAL_QSPI_FLASH_OTP_SECTOR_SIZE_GD25;
        dataToProcess = dataToProcessTotal;

        // If the data remaining to read/write extends into the next register, truncate the read/write amount to fit into this register
        if (register_offset + dataToProcess > HAL_QSPI_FLASH_OTP_SECTOR_SIZE_GD25){
            dataToProcess = HAL_QSPI_FLASH_OTP_SECTOR_SIZE_GD25 - register_offset;
        }

        ret = operation(otpPageIdx, register_offset, &data_buf[dataProcessed], dataToProcess);

        // Increment/decrement counter variables, increment to the next OTP register
        dataToProcessTotal -= dataToProcess;
        dataProcessed += dataToProcess;
        otpPageIdx++;
    }
    return ret;
}

int hal_exflash_read_special(hal_exflash_special_sector_t sp, uintptr_t addr, uint8_t* data_buf, size_t data_size) {
    ExFlashLock lk;

    CHECK_TRUE(sp == HAL_EXFLASH_SPECIAL_SECTOR_OTP, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(data_buf && data_size > 0, SYSTEM_ERROR_INVALID_ARGUMENT);

    if (flash_init_para.FLASH_Id == FLASH_ID_GD) {
        return gd25PerformSecurityRegistersOperation(gd25ReadSecurityRegisters, addr, data_buf, data_size);
    } else if (flash_init_para.FLASH_Id == FLASH_ID_MXIC) {
        // Read the first word at 0x00000000
        uint32_t normalContent = 0;
        FLASH_RxData(0, (uint32_t)0, sizeof(normalContent), (uint8_t*)&normalContent);
        {
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
    } else {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    return SYSTEM_ERROR_NONE;
}

int hal_exflash_write_special(hal_exflash_special_sector_t sp, uintptr_t addr, const uint8_t* data_buf, size_t data_size) {
    ExFlashLock lk;

    CHECK_TRUE(sp == HAL_EXFLASH_SPECIAL_SECTOR_OTP, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(data_buf && data_size > 0, SYSTEM_ERROR_INVALID_ARGUMENT);

    if (flash_init_para.FLASH_Id == FLASH_ID_GD) {
        return gd25PerformSecurityRegistersOperation(gd25WriteSecurityRegisters, addr, (uint8_t*)data_buf, data_size);
    } else if (flash_init_para.FLASH_Id == FLASH_ID_MXIC) {
        // Read the first word at 0x00000000
        uint32_t normalContent = 0;
        FLASH_RxData(0, (uint32_t)0, sizeof(normalContent), (uint8_t*)&normalContent);
        {
            SCOPE_GUARD({
                // Just in case, even if it might have failed to enter the secure OTP mode.
                FLASH_TxCmd(MXIC_FLASH_CMD_EXSO, 0, nullptr);
                if (isSecureOtpMode(normalContent)) {
                    SPARK_ASSERT(false);
                }
            });

            FLASH_TxCmd(MXIC_FLASH_CMD_ENSO, 0, nullptr);
            if (isSecureOtpMode(normalContent)) {
                CHECK(hal_flash_common_write(addr, data_buf, data_size, &perform_write, &hal_flash_common_dummy_read, 0));
            } else {
                return SYSTEM_ERROR_INVALID_STATE;
            }
        }
    } else {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    return SYSTEM_ERROR_NONE;
}

int hal_exflash_erase_special(hal_exflash_special_sector_t sp, uintptr_t addr, size_t size) {
    if (flash_init_para.FLASH_Id == FLASH_ID_GD) {
        ExFlashLock lk(false); // Stop thread scheduler

        size_t otpPage = addr / HAL_QSPI_FLASH_OTP_SECTOR_SIZE_GD25;
        if (otpPage > 2) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        uint32_t address = ((otpPage + 1) << 12);

        RtkSpicFlashConfigGuard guard;

        rtkSpiFlashSendCmd(GENERIC_FLASH_CMD_WREN);
        /* Disable SPI_FLASH */
        SPIC->ssienr = 0;
        rtkSpiSetTxMode();
        /* set flash cmd + addr and write to fifo */
        SPIC->dr[0].word = RTK_SPIC_CMD_ADDR_FORMAT(GD_FLASH_CMD_ERS_SEC, address);
        rtkSpiFlashEnableCsWriteErase();
        rtkSpiFlashSendCmd(GENERIC_FLASH_CMD_WRDI);
        return SYSTEM_ERROR_NONE;
    } else {
        /* We only support OTP sector and since it's One Time Programmable, we can't erase it */
    }
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

#else

// FIXME
int hal_exflash_read_special(hal_exflash_special_sector_t sp, uintptr_t addr, uint8_t* data_buf, size_t data_size) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

int hal_exflash_special_command(hal_exflash_special_sector_t sp, hal_exflash_command_t cmd, const uint8_t* data, uint8_t* result, size_t size) {
    ExFlashLock lk;

    uint8_t byte;

    /* General commands */
    if (sp == HAL_EXFLASH_SPECIAL_SECTOR_NONE) {
        if (cmd == HAL_EXFLASH_COMMAND_SLEEP) {
            byte = GENERIC_FLASH_CMD_DP;
        } else if (cmd == HAL_EXFLASH_COMMAND_WAKEUP) {
            byte = GENERIC_FLASH_CMD_RDID;
        } else if (cmd == HAL_EXFLASH_COMMAND_SUSPEND_PGMERS) {
            byte = GENERIC_FLASH_CMD_PGM_ERS_SPD;
        } else if (cmd == HAL_EXFLASH_COMMAND_RESET) {
            byte = GENERIC_FLASH_CMD_RSTEN;
        } else {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
        // FIXME: The result of the instruction is not garanteed.
        FLASH_TxCmd(byte, 0, nullptr);
        // For the reset procedure, we need to execute the reset command going forward.
        if (cmd == HAL_EXFLASH_COMMAND_RESET) {
            byte = GENERIC_FLASH_CMD_RST;
            // FIXME: The result of the instruction is not garanteed.
            FLASH_TxCmd(byte, 0, nullptr);
        }
    } else if (sp == HAL_EXFLASH_SPECIAL_SECTOR_OTP) {
        if (cmd == HAL_EXFLASH_COMMAND_LOCK_ENTIRE_OTP) {
            byte = GENERIC_FLASH_CMD_WREN;
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

#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
extern "C" int hal_exflash_disable_xip(void) {
    ExFlashLock lk;
    XipControl::initXipOff();
    return SYSTEM_ERROR_NONE;
}
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
