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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#include "logging.h"
#include "spi_hal.h"
#include "pinmap_hal.h"
#include "pinmap_impl.h"
#include "gpio_hal.h"
#include "interrupts_hal.h"
#include "concurrent_hal.h"
#include "delay_hal.h"
#include "check.h"
#include "service_debug.h"
#include <mutex>
#include "flash_common.h"
extern "C" {
#include "rtl8721d.h"
extern void SSI_SetDataSwap(SPI_TypeDef *spi_dev, u32 SwapStatus, u32 newState);
// #include "rtl8721d_ssi.h"
// #include "rtl8721dhp_rcc.h"
}

#include <memory>
#include <cstdlib>
#include <limits>
#include "timer_hal.h"

#define DEFAULT_SPI_MODE        SPI_MODE_MASTER
#define DEFAULT_DATA_MODE       SPI_MODE3
#define DEFAULT_DATA_BITS       DFS_8_BITS
#define DEFAULT_BIT_ORDER       MSBFIRST
#define DEFAULT_SPI_CLOCKDIV    SPI_CLOCK_DIV256

#define CFG_SPI_PRIORITY        6
#define CFG_GDMA_TX_PRIORITY    3
#define CFG_GDMA_RX_PRIORITY    4
#define CFG_CHUNK_BUF_SIZE      32

static_assert((CFG_CHUNK_BUF_SIZE&0x1f) == 0, "Chunk size should be multiple of 32-byte");

namespace {

class Spi;
Spi* getInstance(hal_spi_interface_t spi);

typedef struct {
    hal_spi_mode_t  spiMode;
    uint8_t         dataMode;
    uint8_t         bitOrder;
    uint32_t        clockDiv;
    uint32_t        flags;
    void reset() {
        spiMode  = DEFAULT_SPI_MODE;
        dataMode = DEFAULT_DATA_MODE;
        bitOrder = DEFAULT_BIT_ORDER;
        clockDiv = DEFAULT_SPI_CLOCKDIV;
        flags = 0;
    }
} SpiConfig;

typedef struct {
    const uint8_t* txBuf;
    uint8_t* rxBuf;
    size_t blockSize;
    size_t blockCount;
    size_t remainder;
    void reset() {
        txBuf  = nullptr;
        rxBuf  = nullptr;
        blockSize = 0;
        blockCount = 0;
        remainder = 0;
    }
} SpiTransferConfig;

typedef struct {
    hal_spi_dma_user_callback       dmaUserCb;
    hal_spi_select_user_callback    selectUserCb;
    void reset() {
        dmaUserCb = nullptr;
        selectUserCb = nullptr;
    }
} SpiCallbackConfig;

typedef struct {
    volatile hal_spi_state_t    state;
    volatile bool               receiving;
    volatile bool               transmitting;
    volatile bool               csPinSelected;
    volatile bool               slaveTransferPending;
    volatile uint16_t           transferredLength;
    volatile uint16_t           configuredTransferLength;
    volatile uint16_t           blocksTransferredLength;
} SpiStatus;

void spiModeToPolAndPha(uint32_t spiMode, uint32_t* polarity, uint32_t* phase) {
    /*
    * mode | POL PHA
    * -----+--------
    *   0  |  0   0
    *   1  |  0   1
    *   2  |  1   0
    *   3  |  1   1
    *
    * SCPOL_INACTIVE_IS_LOW  = 0,
    * SCPOL_INACTIVE_IS_HIGH = 1
    *
    * SCPH_TOGGLES_IN_MIDDLE = 0,
    * SCPH_TOGGLES_AT_START  = 1
    */
    switch (spiMode) {
        case SPI_MODE0: {
            *polarity = SCPOL_INACTIVE_IS_LOW;
            *phase    = SCPH_TOGGLES_IN_MIDDLE;
            break;
        };
        case SPI_MODE1: {
            *polarity = SCPOL_INACTIVE_IS_LOW;
            *phase    = SCPH_TOGGLES_AT_START;
            break;
        };
        case SPI_MODE2: {
            *polarity = SCPOL_INACTIVE_IS_HIGH;
            *phase    = SCPH_TOGGLES_IN_MIDDLE;
            break;
        };
        case SPI_MODE3: {
            *polarity = SCPOL_INACTIVE_IS_HIGH;
            *phase    = SCPH_TOGGLES_AT_START;
            break;
        };
        default:
            // Shouldn't enter this case after parameter check
            *polarity = SCPOL_INACTIVE_IS_HIGH;
            *phase    = SCPH_TOGGLES_AT_START;
            break;
    }
}

void clockDivToRtlClockDiv(uint32_t clockDiv, uint32_t* rtlClockDivider) {
    switch (clockDiv) {
        case SPI_CLOCK_DIV2:   *rtlClockDivider = 2;   break;
        case SPI_CLOCK_DIV4:   *rtlClockDivider = 4;   break;
        case SPI_CLOCK_DIV8:   *rtlClockDivider = 8;   break;
        case SPI_CLOCK_DIV16:  *rtlClockDivider = 16;  break;
        case SPI_CLOCK_DIV32:  *rtlClockDivider = 32;  break;
        case SPI_CLOCK_DIV64:  *rtlClockDivider = 64;  break;
        case SPI_CLOCK_DIV128: *rtlClockDivider = 128; break;
        case SPI_CLOCK_DIV256: *rtlClockDivider = 256; break;
        default:               *rtlClockDivider = 256; break;
    }
}

class Spi {
public:
    Spi(hal_spi_interface_t spiInterface, int rtlSpiIndex, uint32_t spiInputClock, int prio,
            hal_pin_t csPin, hal_pin_t clkPin, hal_pin_t mosiPin, hal_pin_t misoPin)
            : spiInterface_(spiInterface),
              rtlSpiIndex_(rtlSpiIndex),
              spiInputClock_(spiInputClock),
              prio_(prio),
              csPin_(csPin),
              sclkPin_(clkPin),
              mosiPin_(mosiPin),
              misoPin_(misoPin),
              config_{},
              transferConfig_{},
              callbackConfig_{},
              ssPinState_{},
              rxDmaInitStruct_{},
              txDmaInitStruct_{},
              mutex_{},
              status_{} {
        config_.reset();
    }

    int init() {
        os_thread_scheduling(false, nullptr);
        if (mutex_ == nullptr) {
            os_mutex_recursive_create(&mutex_);
        }
        os_thread_scheduling(true, nullptr);

        // Enable SPI Clock
        if (rtlSpiIndex_ == 0) {
            RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);
        } else {
            RCC_PeriphClockCmd(APBPeriph_SPI1, APBPeriph_SPI1_CLOCK, ENABLE);
        }

        std::lock_guard<Spi> lk(*this);
        if (isEnabled()) {
            CHECK(end());
        }

        config_.reset();
        transferConfig_.reset();
        return SYSTEM_ERROR_NONE;
    }

    int deinit() {
        end();
        os_mutex_recursive_destroy(&mutex_);
        return SYSTEM_ERROR_NONE;
    }

    int setSettings(const SpiConfig& config, const hal_spi_config_t* spiConfig = nullptr, bool force = false) {
        CHECK_TRUE(validateConfig(rtlSpiIndex_, config), SYSTEM_ERROR_INVALID_ARGUMENT);
        // Save config
        memcpy(&config_, &config, sizeof(SpiConfig));
        if (spiConfig && spiConfig->flags) {
            config_.flags = spiConfig->flags;
        }
        // Set data mode
        setDataMode(config_.dataMode, force);
        // Set clock divider
        setClockDivider(config_.clockDiv);
        // Set bit order
        setBitOrder(config_.bitOrder);
        return SYSTEM_ERROR_NONE;
    }

    int setBitOrder(uint8_t bitOrder) {
        config_.bitOrder = bitOrder;
        if (SPI_DEV_TABLE[rtlSpiIndex_].SPIx->SSIENR & BIT_SSIENR_SSI_EN) {
            if (config_.bitOrder == MSBFIRST) {
                SSI_SetDataSwap(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, BIT_CTRLR0_TXBITSWP, DISABLE);
                SSI_SetDataSwap(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, BIT_CTRLR0_RXBITSWP, DISABLE);
            } else {
                SSI_SetDataSwap(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, BIT_CTRLR0_TXBITSWP, ENABLE);
                SSI_SetDataSwap(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, BIT_CTRLR0_RXBITSWP, ENABLE);
            }
        }
        return SYSTEM_ERROR_NONE;
    }

    int setDataMode(uint8_t dataMode, bool force = false) {
        config_.dataMode = dataMode;
        if (SPI_DEV_TABLE[rtlSpiIndex_].SPIx->SSIENR & BIT_SSIENR_SSI_EN) {
            uint32_t phase = SCPH_TOGGLES_IN_MIDDLE;
            uint32_t polarity = SCPOL_INACTIVE_IS_LOW;
            spiModeToPolAndPha(config_.dataMode, &polarity, &phase);
            uint32_t bakPol = SPI_DEV_TABLE[rtlSpiIndex_].SPIx->CTRLR0 & BIT_CTRLR0_SCPH;
            SSI_SetSclkPhase(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, phase);
            SSI_SetSclkPolarity(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, polarity);
            if (force || (bakPol != (SPI_DEV_TABLE[rtlSpiIndex_].SPIx->CTRLR0 & BIT_CTRLR0_SCPH))) {
                // PAD_PullCtrl is another slow call, avoid if settings still match
                if (polarity == SCPOL_INACTIVE_IS_LOW) {
                    PAD_PullCtrl((uint8_t)hal_pin_to_rtl_pin(sclkPin_), GPIO_PuPd_DOWN);
                } else {
                    PAD_PullCtrl((uint8_t)hal_pin_to_rtl_pin(sclkPin_), GPIO_PuPd_UP);
                }
            }
        }
        return SYSTEM_ERROR_NONE;
    }

    int setClockDivider(uint32_t clockDiv) {
        config_.clockDiv = clockDiv;
        if (SPI_DEV_TABLE[rtlSpiIndex_].SPIx->SSIENR & BIT_SSIENR_SSI_EN) {
            u32 rtlClockDivider = 256;
            clockDivToRtlClockDiv(config_.clockDiv, &rtlClockDivider);
            SSI_SetBaudDiv(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, rtlClockDivider);
            // Set sample delay for SPI0@50MHz/25MHz
            if (rtlClockDivider <= 4 && SPI_DEV_TABLE[rtlSpiIndex_].SPIx == SPI0_DEV) {
                SSI_SetSampleDelay(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, 0x1);
            } else {
                SSI_SetSampleDelay(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, 0x0);
            }
        }
        return SYSTEM_ERROR_NONE;
    }

    int begin(const SpiConfig& config) {
        return begin(csPin_, config, nullptr);
    }

    int begin(uint16_t csPin, const SpiConfig& config, const hal_spi_config_t* spi_config) {
        CHECK_TRUE(validateConfig(rtlSpiIndex_, config), SYSTEM_ERROR_INVALID_ARGUMENT);

        // Convert default pin to exact pin
        if (csPin == SPI_DEFAULT_SS) {
            if (spiInterface_ == HAL_SPI_INTERFACE1) {
                csPin = SS; 
            } else if (spiInterface_ == HAL_SPI_INTERFACE2) {
                csPin = SS1;
            } else {
                csPin = PIN_INVALID;
            }
        }
        // SPI slave mode doesn't allow invalid cs pin
        if ((config.spiMode == SPI_MODE_SLAVE) && !hal_pin_is_valid(csPin)) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }

        if (isEnabled()) {
            if (config_.spiMode == config.spiMode && config_.flags == (spi_config ? spi_config->flags : 0)
                    && csPin == csPin_) {
                // There is no need to reconfigure gpio, which is a pretty slow operation if settings match
                return setSettings(config);
            }
            CHECK(end());
        }

        csPin_ = csPin;

        // Allocate dma channels
        CHECK(initDmaChannels());

        // Save config
        memcpy(&config_, &config, sizeof(SpiConfig));
        if (spi_config && spi_config->flags) {
            config_.flags = spi_config->flags;
        }

        SSI_InitTypeDef SSI_InitStruct;
        SSI_StructInit(&SSI_InitStruct);

        // Configure GPIO and Role
        if (config_.spiMode == SPI_MODE_MASTER) {
            SSI_InitStruct.SPI_Role = SSI_MASTER;
            // NOTE: Extra setting for SPI0_DEV, please refer to rtl8721dhp_ssi.c
            if (rtlSpiIndex_ == 0) {
                SSI_SetRole(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, SSI_MASTER);
            }
            // FIXME: a noise is observed on the MOSI pin, adding a pullup here is not ideal,
            // fix it when we find a better way e.g. improve driving strength
            hal_gpio_mode(mosiPin_, INPUT_PULLUP);
            if (!(spi_config && (spi_config->flags & (uint32_t)HAL_SPI_CONFIG_FLAG_MOSI_ONLY))) {
                Pinmux_Config((uint8_t)hal_pin_to_rtl_pin(sclkPin_), PINMUX_FUNCTION_SPIM);
                Pinmux_Config((uint8_t)hal_pin_to_rtl_pin(misoPin_), PINMUX_FUNCTION_SPIM);
            }
            Pinmux_Config((uint8_t)hal_pin_to_rtl_pin(mosiPin_), PINMUX_FUNCTION_SPIM);

            hal_gpio_write(csPin_, 1);
            hal_gpio_mode(csPin_, OUTPUT);
        } else {
            SSI_InitStruct.SPI_Role = SSI_SLAVE;
            // NOTE: Extra setting for SPI0_DEV, please refer to rtl8721dhp_ssi.c
            if (rtlSpiIndex_ == 0) {
                SSI_SetRole(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, SSI_SLAVE);
            }
            if (!(spi_config && (spi_config->flags & (uint32_t)HAL_SPI_CONFIG_FLAG_MOSI_ONLY))) {
                Pinmux_Config((uint8_t)hal_pin_to_rtl_pin(sclkPin_), PINMUX_FUNCTION_SPIS);
                Pinmux_Config((uint8_t)hal_pin_to_rtl_pin(misoPin_), PINMUX_FUNCTION_SPIS);
            }
            Pinmux_Config((uint8_t)hal_pin_to_rtl_pin(mosiPin_), PINMUX_FUNCTION_SPIS);
            hal_gpio_mode(csPin_, INPUT_PULLUP);
            hal_interrupt_attach(csPin_, onSelectedHandler, this, CHANGE, nullptr);
        }
        SSI_Init(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, &SSI_InitStruct);
        if (config_.spiMode == SPI_MODE_MASTER) {
            hal_gpio_set_drive_strength(sclkPin_, HAL_GPIO_DRIVE_HIGH);
            hal_gpio_set_drive_strength(mosiPin_, HAL_GPIO_DRIVE_HIGH);
            hal_gpio_set_drive_strength(misoPin_, HAL_GPIO_DRIVE_HIGH);
        } else {
            hal_gpio_set_drive_strength(sclkPin_, HAL_GPIO_DRIVE_STANDARD);
            hal_gpio_set_drive_strength(mosiPin_, HAL_GPIO_DRIVE_STANDARD);
            hal_gpio_set_drive_strength(misoPin_, HAL_GPIO_DRIVE_STANDARD);
        }

        // Set data mode
        setDataMode(config_.dataMode, true /* force */);
        // Set clock divider
        setClockDivider(config_.clockDiv);
        // Set bit order
        setBitOrder(config_.bitOrder);

        // Set pin function
        if (!(spi_config && (spi_config->flags & (uint32_t)HAL_SPI_CONFIG_FLAG_MOSI_ONLY))) {
            hal_pin_set_function(sclkPin_, PF_SPI);
            hal_pin_set_function(misoPin_, PF_SPI);
        }
        hal_pin_set_function(mosiPin_, PF_SPI);

        // DMA is not enabled immediately
        // SSI_SetDmaEnable(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, ENABLE, BIT_SHIFT_DMACR_TDMAE);
        // SSI_SetDmaEnable(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, ENABLE, BIT_SHIFT_DMACR_RDMAE);

        // We don't use SPI interrupt
        SSI_INTConfig(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, (BIT_IMR_RXFIM | BIT_IMR_RXOIM | BIT_IMR_RXUIM), DISABLE);
        SSI_INTConfig(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, (BIT_IMR_TXOIM | BIT_IMR_TXEIM), DISABLE);
        InterruptRegister(interruptHandler, SPI_DEV_TABLE[rtlSpiIndex_].IrqNum, (u32)this, prio_);
        NVIC_ClearPendingIRQ(SPI_DEV_TABLE[rtlSpiIndex_].IrqNum);
        InterruptDis(SPI_DEV_TABLE[rtlSpiIndex_].IrqNum);

        // Clear dma interrupt just in case
        GDMA_ClearINT(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum);
        GDMA_ClearINT(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum);

        // Update state
        status_.state = HAL_SPI_STATE_ENABLED;

        return SYSTEM_ERROR_NONE;
    }

    int end(bool ignoreGpio = false) {
        CHECK_TRUE(status_.state == HAL_SPI_STATE_ENABLED, SYSTEM_ERROR_INVALID_STATE);

        // Wait for last SPI transfer finished
        while (isBusy()) {
            ;
        }

        /* Set SSI DMA Disable */
        SSI_SetDmaEnable(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, DISABLE, BIT_SHIFT_DMACR_RDMAE);
        SSI_SetDmaEnable(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, DISABLE, BIT_SHIFT_DMACR_TDMAE);

        status_.slaveTransferPending = false;
        transferDmaCancel();

        if (!ignoreGpio) {
            GDMA_ChnlFree(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum);
            GDMA_ChnlFree(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum);
        }

        SSI_Cmd(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, DISABLE);

        // Set GPIO, pin function and pinmux
        if (!ignoreGpio) {
            hal_gpio_mode(sclkPin_, PIN_MODE_NONE);
            hal_gpio_mode(mosiPin_, PIN_MODE_NONE);
            hal_gpio_mode(misoPin_, PIN_MODE_NONE);
            hal_pin_set_function(sclkPin_, PF_NONE);
            hal_pin_set_function(mosiPin_, PF_NONE);
            hal_pin_set_function(misoPin_, PF_NONE);
            Pinmux_Config(hal_pin_to_rtl_pin(sclkPin_), PINMUX_FUNCTION_GPIO);
            Pinmux_Config(hal_pin_to_rtl_pin(mosiPin_), PINMUX_FUNCTION_GPIO);
            Pinmux_Config(hal_pin_to_rtl_pin(misoPin_), PINMUX_FUNCTION_GPIO);
            hal_gpio_set_drive_strength(sclkPin_, HAL_GPIO_DRIVE_DEFAULT);
            hal_gpio_set_drive_strength(mosiPin_, HAL_GPIO_DRIVE_DEFAULT);
            hal_gpio_set_drive_strength(misoPin_, HAL_GPIO_DRIVE_DEFAULT);
        }
        if (config_.spiMode == SPI_MODE_SLAVE) {
            hal_interrupt_detach(csPin_);
        }
        // Update state
        status_.state = HAL_SPI_STATE_DISABLED;
        status_.transferredLength = 0;

        return SYSTEM_ERROR_NONE;
    }

    int lock(system_tick_t timeout = 1) {
        CHECK_TRUE(hal_interrupt_is_isr() == false, SYSTEM_ERROR_INVALID_STATE);
        // FIXME: os_mutex_recursive_lock doesn't take any arguments, using trylock for now
        if (timeout != 0) {
            return os_mutex_recursive_lock(mutex_);
        }
        return os_mutex_recursive_trylock(mutex_);
    }

    int unlock() {
        CHECK_TRUE(hal_interrupt_is_isr() == false, SYSTEM_ERROR_INVALID_STATE);
        return os_mutex_recursive_unlock(mutex_);
    }

    uint8_t transfer(uint8_t data) {
        CHECK_TRUE(isEnabled(), 0);
        CHECK_TRUE(config_.spiMode == SPI_MODE_MASTER, 0);

        // Wait for last SPI transfer finished
        while (isBusy()) {
            ;
        }

        // Wait until SPI is writeable
        while (!SSI_Writeable(SPI_DEV_TABLE[rtlSpiIndex_].SPIx)) {
            ;
        }
        SSI_WriteData(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, data);

        // Wait until SPI is readable
        while (!SSI_Readable(SPI_DEV_TABLE[rtlSpiIndex_].SPIx)) {
            ;
        }

        return (uint8_t)SSI_ReadData(SPI_DEV_TABLE[rtlSpiIndex_].SPIx);
    }

    static size_t findDmaBlockSize(size_t length) {
        if (length <= 0xfff) {
            return length;
        }

        // Divides evenly
        for (size_t i = 2; i < 0xfff && i <= length / 2; i++) {
            if (length % i != 0) {
                continue;
            }
            size_t block = length / i;
            // Arbitrary
            if (block <= 32) {
                break;
            }
            if (block <= 0xfff) {
                return block;
            }
        }
        // With remainder
        for (size_t i = 2; i < 0xfff && i <= length / 2; i++) {
            size_t block = length / i;
            if (block <= 0xfff) {
                return block;
            }
        }
        return 0;
    }

    int startDma() {
        // Dummy
        static uint32_t dummy __attribute__((aligned(4))) = 0xffffffff;
        // We need to reset the dummy buffer to send 0xff, cos it may be used for RX previously.
        memset(&dummy, 0xff, sizeof(dummy));
        DCache_CleanInvalidate((u32)&dummy, sizeof(dummy));
        uint8_t* txBuf = transferConfig_.txBuf ? (uint8_t*)transferConfig_.txBuf + status_.transferredLength : (uint8_t*)&dummy;
        uint8_t* rxBuf = transferConfig_.rxBuf ? (uint8_t*)transferConfig_.rxBuf + status_.transferredLength : (uint8_t*)&dummy;

        size_t blockSize = transferConfig_.blockSize;
        size_t blockCount = transferConfig_.blockCount;
        bool multiBlock = transferConfig_.blockCount > 1;
        if (status_.transferredLength > 0) {
            blockSize = transferConfig_.remainder;
            blockCount = 0;
            multiBlock = false;
        }

        txDmaInitStruct_.GDMA_SrcAddr = (u32)txBuf;
        if (!transferConfig_.txBuf) {
            txDmaInitStruct_.GDMA_ReloadSrc = 1;
            txDmaInitStruct_.GDMA_SrcInc = NoChange;
        } else {
            txDmaInitStruct_.GDMA_ReloadSrc = 0;
            txDmaInitStruct_.GDMA_SrcInc = IncType;
        }
        if (((blockSize & 0x03) == 0) && (((uintptr_t)txBuf & 0x03) == 0) && blockSize >= 4) {
            /*  4-bytes aligned, move 4 bytes each transfer */
            txDmaInitStruct_.GDMA_SrcMsize = MsizeOne;
            txDmaInitStruct_.GDMA_SrcDataWidth = TrWidthFourBytes;
            txDmaInitStruct_.GDMA_BlockSize = blockSize >> 2;
        } else {
            txDmaInitStruct_.GDMA_SrcMsize = MsizeFour;
            txDmaInitStruct_.GDMA_SrcDataWidth = TrWidthOneByte;
            txDmaInitStruct_.GDMA_BlockSize = blockSize;
        }

        rxDmaInitStruct_.GDMA_DstAddr = (u32)rxBuf;
        if (!transferConfig_.rxBuf) {
            rxDmaInitStruct_.GDMA_ReloadDst = 1;
            rxDmaInitStruct_.GDMA_DstInc = NoChange;
        } else {
            rxDmaInitStruct_.GDMA_ReloadDst = 0;
            rxDmaInitStruct_.GDMA_DstInc = IncType;
        }
        if (((blockSize & 0x03) == 0) && (((uintptr_t)rxBuf & 0x03) == 0) && blockSize >= 4) {
            /*  4-bytes aligned, move 4 bytes each transfer */
            rxDmaInitStruct_.GDMA_DstMsize = MsizeOne;
            rxDmaInitStruct_.GDMA_DstDataWidth = TrWidthFourBytes;
        } else {
            rxDmaInitStruct_.GDMA_DstMsize = MsizeFour;
            rxDmaInitStruct_.GDMA_DstDataWidth = TrWidthOneByte;
        }
        rxDmaInitStruct_.GDMA_BlockSize = blockSize;

        if (multiBlock) {
            txDmaInitStruct_.MaxMuliBlock = blockCount;
            rxDmaInitStruct_.MaxMuliBlock = blockCount;
        } else {
            txDmaInitStruct_.MaxMuliBlock = 0;
            rxDmaInitStruct_.MaxMuliBlock = 0;
        }
        txDmaInitStruct_.MuliBlockCunt = 0;
        rxDmaInitStruct_.MuliBlockCunt = 0;

        if (transferConfig_.txBuf) {
            DCache_CleanInvalidate((u32)transferConfig_.txBuf, status_.configuredTransferLength);
        }
        if (transferConfig_.rxBuf) {
            // This is required to make sure that software writes into rx buffer
            // are finalized, otherwise after DMA completes Invalidate may result
            // in garbage data in rxBuf.
            DCache_CleanInvalidate((u32)transferConfig_.rxBuf, status_.configuredTransferLength);
        }

        /*  Enable GDMA for RX */
        GDMA_Init(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, &rxDmaInitStruct_);
        GDMA_Cmd(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, ENABLE);
        status_.receiving = true;
        SSI_SetDmaEnable(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, ENABLE, BIT_SHIFT_DMACR_RDMAE);

        /*  Enable GDMA for TX */
        GDMA_Init(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, &txDmaInitStruct_);
        GDMA_Cmd(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, ENABLE);
        status_.transmitting = true;
        SSI_SetDmaEnable(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, ENABLE, BIT_SHIFT_DMACR_TDMAE);
        // NOTE: TX DMA is started even if master hasn't generated clock.
        return 0;
    }

    int transferDma(const uint8_t* txBuf, uint8_t* rxBuf, size_t size, hal_spi_dma_user_callback callback) {
        CHECK_TRUE((txBuf || rxBuf) && size > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(isEnabled(), SYSTEM_ERROR_INVALID_STATE);

        // Wait for last SPI master transfer finished
        while ((config_.spiMode == SPI_MODE_MASTER) && isBusy()) {
            ;
        }

        // Reset SPI in case the data in the buffer is not read completely in the last transfer
        transferDmaCancel();

        transferConfig_.reset();

        // Hardware DMA peripheral limit of 4095
        size_t blockSize = findDmaBlockSize(size);
        if (blockSize == 0) {
            return SYSTEM_ERROR_OUT_OF_RANGE;
        }
        size_t blockCount = size / blockSize;
        if (blockCount > 0xfff) {
            return SYSTEM_ERROR_TOO_LARGE;
        }
        size_t remainder = size - (blockCount * blockSize);

        transferConfig_.rxBuf = rxBuf;
        transferConfig_.txBuf = txBuf;
        transferConfig_.blockSize = blockSize;
        transferConfig_.blockCount = blockCount;
        transferConfig_.remainder = remainder;

        callbackConfig_.dmaUserCb = callback;
        status_.configuredTransferLength = size;
        status_.transferredLength = 0;
        status_.blocksTransferredLength = 0;
        status_.slaveTransferPending = false;

        return startDma();
    }

    int transferDmaCancel(bool flushTx = false) {
        /* Clear Pending ISR */
        GDMA_ChCleanAutoReload(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, CLEAN_RELOAD_SRC_DST);
        GDMA_ChCleanAutoReload(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, CLEAN_RELOAD_SRC_DST);
        GDMA_Cmd(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, DISABLE);
        GDMA_Cmd(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, DISABLE);
        GDMA_ClearINT(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum);
        GDMA_ClearINT(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum);

        flushRxFifo();

        if (config_.spiMode == SPI_MODE_SLAVE || flushTx) {
            SSI_Cmd(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, DISABLE);
            SSI_Cmd(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, ENABLE);
        }

        if (config_.spiMode == SPI_MODE_SLAVE && status_.slaveTransferPending) {
            status_.transmitting = true;
            status_.receiving = true;
        } else {
            status_.transmitting = false;
            status_.receiving = false;
            transferConfig_.txBuf = nullptr;
            transferConfig_.rxBuf = nullptr;
            callbackConfig_.dmaUserCb = nullptr;
        }

        return SYSTEM_ERROR_NONE;
    }

    void flushRxFifo() {
        // Flush fifo
        while (SSI_Readable(SPI_DEV_TABLE[rtlSpiIndex_].SPIx)) {
            u32 rxFifoLevel = SSI_GetRxCount(SPI_DEV_TABLE[rtlSpiIndex_].SPIx);
            for (u32 i = 0; i < rxFifoLevel; i++) {
                uint8_t c = SSI_ReadData(SPI_DEV_TABLE[rtlSpiIndex_].SPIx);
                if (config_.spiMode == SPI_MODE_SLAVE && transferConfig_.rxBuf && status_.transferredLength < status_.configuredTransferLength) {
                    transferConfig_.rxBuf[status_.transferredLength++] = c;
                }
            }
        }
    }

    SpiConfig config() const {
        return config_;
    }

    SpiStatus status() const {
        return status_;
    }

    uint32_t getSpiInputClock() const {
        return spiInputClock_;
    }

    hal_pin_t csPin() const {
        return csPin_;
    }

    bool isEnabled() const {
        return status_.state == HAL_SPI_STATE_ENABLED;
    }

    // FIXME: The TX DMA exception is notified just when the source data
    // is transferred to the SPI FIFO, not when the data is actually clocked out.
    // Whereas the RX DMA exception is notified when the data is actually clocked in and out.
    // When the RX buffer is not supplied, we may lose some TX data due to the false SPI completion.
    // Hence, we add the SSI_Busy() check here.
    bool isBusy() const {
        return status_.transmitting || status_.receiving || ((config_.spiMode == SPI_MODE_MASTER) && SSI_Busy(SPI_DEV_TABLE[rtlSpiIndex_].SPIx));
    }

    bool isSuspended() const {
        return status_.state == HAL_SPI_STATE_SUSPENDED;
    }

    bool isDmaBufferConfigured() const {
        // Use at least one buffer for SPI transfer
        return transferConfig_.txBuf || transferConfig_.rxBuf;
    }

    int registerSelectUserCb(hal_spi_select_user_callback callback) {
        callbackConfig_.selectUserCb = callback;
        return SYSTEM_ERROR_NONE;
    }

    void interruptHandlerImpl() {
        u32 interruptStatus = SSI_GetIsr(SPI_DEV_TABLE[rtlSpiIndex_].SPIx);
        SSI_SetIsrClean(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, interruptStatus);

        if (interruptStatus & (BIT_ISR_TXOIS | BIT_ISR_RXUIS | BIT_ISR_RXOIS | BIT_ISR_MSTIS)) {
            LOG_DEBUG(WARN, "SPI error 0x%08x", interruptStatus);
        }
    }

    void dmaTxHandlerImpl(bool forceStop = false) {
        bool doneWithBlocks = forceStop;
        txDmaInitStruct_.MuliBlockCunt++;

        // IMPORTANT: needs to be caught here before any of the GDMA calls are done
        if (transferConfig_.txBuf) {
            // In case that transferConfig_.rxBuf is nullptr
            uint32_t p = GDMA_GetSrcAddr(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum);
            status_.blocksTransferredLength = p - txDmaInitStruct_.GDMA_SrcAddr;
        }

        if (!txDmaInitStruct_.MaxMuliBlock || txDmaInitStruct_.MuliBlockCunt == txDmaInitStruct_.MaxMuliBlock) {
            GDMA_Cmd(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, DISABLE);
            doneWithBlocks = true;
        }

        // Clear Pending ISR, free TX DMA resource
        uint32_t isrTypeMap = GDMA_ClearINT(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum);
        (void)isrTypeMap;

        if (doneWithBlocks) {
            SSI_SetDmaEnable(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, DISABLE, BIT_SHIFT_DMACR_TDMAE);
            status_.transmitting = false;
        }
    }

    void flushDmaRxFiFo() {
        constexpr uint32_t BIT_CFGX_LO_FIFO_EMPTY = (1 << 9);
        constexpr uint32_t BIT_CFGX_LO_CH_SUSP = (1 << 8);

        if ((GDMA_BASE->CH[rxDmaInitStruct_.GDMA_ChNum].CFG_LOW & BIT_CFGX_LO_FIFO_EMPTY) == 0) {
            // Suspending DMA channel forces flushing of data into destination from GDMA FIFO
            // NOTE: The datasheet says that "there is no guarantee that the current transaction will complete", which is why we enter the busy loop below in these cases
            GDMA_BASE->CH[rxDmaInitStruct_.GDMA_ChNum].CFG_LOW |= BIT_CFGX_LO_CH_SUSP;
            __DSB();
            __ISB();
            // while ((GDMA_BASE->ChEnReg & (1 << rxDmaInitStruct_.GDMA_ChNum))) {
            //     // This loop is intended to delay until the DMA controller transfers the expected number of bytes, based off of the address returned in GDMA_GetDstAddr()
            //     // When a DMA transaction is near the end of a block (within the last 4 bytes), sometimes the address returned does not increment to the expected destination. In some cases it resets completely.
            //     // To work around this, we poll the ChEnReg bit for the RX DMA channel. The data sheet says it is: "automatically cleared by hardware when the DMA transfer to the destination is complete".
            //     // The assumption is that if we see this bit is cleared, then the transfer is complete, the data we wanted flushed is available, there is no point to waiting any longer and we can return.
            // }
            GDMA_BASE->CH[rxDmaInitStruct_.GDMA_ChNum].CFG_LOW &= ~(BIT_CFGX_LO_CH_SUSP);
            __DSB();
            __ISB();
        }
    }

    void dmaRxHandlerImpl(bool forceStop = false) {
        // Clear Pending ISR, free RX DMA resource
        rxDmaInitStruct_.MuliBlockCunt++;

        bool doneWithBlocks = forceStop;
        bool doneWithTransfer = true;

        if (forceStop) {
            flushDmaRxFiFo();
        }

        // IMPORTANT: needs to be caught here before any of the GDMA calls are done
        if (transferConfig_.rxBuf) {
            uint32_t p = GDMA_GetDstAddr(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum);
            status_.blocksTransferredLength = p - rxDmaInitStruct_.GDMA_DstAddr;
        }

        if (!rxDmaInitStruct_.MaxMuliBlock || rxDmaInitStruct_.MuliBlockCunt == rxDmaInitStruct_.MaxMuliBlock) {
            GDMA_Cmd(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, DISABLE);
            doneWithBlocks = true;
        }

        uint32_t isrTypeMap = GDMA_ClearINT(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum);
        (void)isrTypeMap;

        if (doneWithBlocks) {
            SSI_SetDmaEnable(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, DISABLE, BIT_SHIFT_DMACR_RDMAE);
            status_.receiving = false;
            status_.transferredLength += status_.blocksTransferredLength;

            if (forceStop) {
                // We cannot rely on the TX DMA to calculate the transferred length,
                // because TX DMA starts moving data when CS is selected, even if there is no clock generated by Master.
                if (!transferConfig_.rxBuf) {
                    // FIXME: It's possible that the FIFO is just flushed by DMA
                    if (SSI_Readable(SPI_DEV_TABLE[rtlSpiIndex_].SPIx) == 0) {
                        // There is no clock generated by Master. The CS pin is just selected and deselected.
                        // We'll leave the buffer configuration as is and starts the DMA transfer on next CS pin being selected.
                        status_.transferredLength = 0;
                    }
                }
                if (status_.transferredLength == 0) {
                    status_.slaveTransferPending = true;
                    doneWithTransfer = false;
                }
            } else {
                if ((status_.transferredLength < status_.configuredTransferLength) && transferConfig_.remainder) {
                    // Transfer the remainder
                    startDma();
                    return;
                }
            }

            if (transferConfig_.rxBuf) {
                DCache_Invalidate((u32)transferConfig_.rxBuf, status_.configuredTransferLength);
            }
            flushRxFifo();

            if (doneWithTransfer) {
                // At least some of the data is transferred. Mark this transfer as completed.
                if (callbackConfig_.dmaUserCb) {
                    (*callbackConfig_.dmaUserCb)();
                }
            }
        }
    }

    void onSelectedHandlerImpl() {
        status_.csPinSelected = !hal_gpio_read(csPin_);
        if (callbackConfig_.selectUserCb && status_.csPinSelected) {
            (*callbackConfig_.selectUserCb)(status_.csPinSelected);
            // XXX: This is required to fixed the issue that the unaligned part of the
            // RX buffer is overwritten with stale data
            if (transferConfig_.rxBuf) {
                DCache_Invalidate((u32)transferConfig_.rxBuf, status_.configuredTransferLength);
            }
        }

        if (isDmaBufferConfigured()) {
            if (status_.csPinSelected) {
                if (status_.slaveTransferPending) {
                    status_.slaveTransferPending = false;
                    status_.transferredLength = 0;
                    startDma();
                }
            } else {
                GDMA_INTConfig(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, 0x1f, DISABLE);
                GDMA_INTConfig(txDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, 0x1f, DISABLE);
                if (status_.transmitting) {
                    dmaTxHandlerImpl(true);
                }
                if (status_.receiving) {
                    dmaRxHandlerImpl(true);
                }
                transferDmaCancel(true);
                // FIXME: When configured as Slave role and DMA buffer is not configured and the Master is clocking data,
                // MISO will present the first byte of the last configured TX buffer, even if the following
                // conditions are true:
                // SSI_SetDmaEnable(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, DISABLE, BIT_SHIFT_DMACR_TDMAE);
                // GDMA_Cmd(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, DISABLE);
            }
        }
        // When the RX buffer is not 32-bytes aligned and part of it is within the same 32-bytes block
        // as certain volatile variables (e.g. volatile flag to be used in user callback), we should make
        // sure the RX buffer is invalidated before those variables are modified (potentially a DCache
        // clean operation is performed), otherwise the unaligned part of the RX buffer may be written
        // with stale data when updating the volatile variables.
        if (callbackConfig_.selectUserCb && !status_.csPinSelected) {
            (*callbackConfig_.selectUserCb)(status_.csPinSelected);
        }
    }

    static bool validateConfig(int rtlSpiIndex, const SpiConfig& config) {
        CHECK_TRUE(config.spiMode == SPI_MODE_MASTER ||
                   config.spiMode == SPI_MODE_SLAVE, false);
        CHECK_TRUE(config.dataMode == SPI_MODE0 ||
                   config.dataMode == SPI_MODE1 ||
                   config.dataMode == SPI_MODE2 ||
                   config.dataMode == SPI_MODE3, false);
        CHECK_TRUE(config.bitOrder == MSBFIRST ||
                   config.bitOrder == LSBFIRST, false);
        // SPI0 can work as master and slave while SPI1 can only work as master.
        CHECK_FALSE(rtlSpiIndex == 1 && config.spiMode == SPI_MODE_SLAVE, false);
        return true;
    }

    static uint32_t interruptHandler(void* context) {
        Spi* spiInstance = (Spi*)context;
        spiInstance->interruptHandlerImpl();
        return SYSTEM_ERROR_NONE;
    }

    static u32 dmaTxHandler(void* context) {
        Spi* spiInstance = (Spi*)context;
        spiInstance->dmaTxHandlerImpl();
        return 0;
    }

    static u32 dmaRxHandler(void* context) {
        Spi* spiInstance = (Spi*)context;
        spiInstance->dmaRxHandlerImpl();
        return 0;
    }

    static void onSelectedHandler(void* context) {
        Spi* spiInstance = (Spi*)context;
        spiInstance->onSelectedHandlerImpl();
    }

private:
    int initDmaChannels() {
        uint8_t txGdmaChannel = GDMA_ChnlAlloc(0, dmaTxHandler, (uint32_t)this, CFG_GDMA_TX_PRIORITY);
        if (txGdmaChannel == 0xFF) {
            return SYSTEM_ERROR_INTERNAL;
        }
        uint8_t rxGdmaChannel = GDMA_ChnlAlloc(0, dmaRxHandler, (uint32_t)this, CFG_GDMA_RX_PRIORITY);
        if (rxGdmaChannel == 0xFF) {
            GDMA_ChnlFree(0, txGdmaChannel);
            return SYSTEM_ERROR_INTERNAL;
        }

        // The below settings won't change over time
        _memset(&txDmaInitStruct_, 0, sizeof(GDMA_InitTypeDef));
        txDmaInitStruct_.GDMA_DIR = TTFCMemToPeri;
        txDmaInitStruct_.GDMA_ReloadDst = 1;
        txDmaInitStruct_.GDMA_DstHandshakeInterface = SPI_DEV_TABLE[rtlSpiIndex_].Tx_HandshakeInterface;
        txDmaInitStruct_.GDMA_DstAddr = (u32)&SPI_DEV_TABLE[rtlSpiIndex_].SPIx->DR;
        txDmaInitStruct_.GDMA_DstInc = NoChange;
        txDmaInitStruct_.GDMA_DstMsize = MsizeFour;
        txDmaInitStruct_.GDMA_DstDataWidth = TrWidthOneByte;
        txDmaInitStruct_.GDMA_Index = 0;
        txDmaInitStruct_.GDMA_ChNum = txGdmaChannel;
        txDmaInitStruct_.GDMA_IsrType = (BlockType|TransferType|ErrType);
        
        _memset(&rxDmaInitStruct_, 0, sizeof(GDMA_InitTypeDef));
        rxDmaInitStruct_.GDMA_DIR = TTFCPeriToMem;
        rxDmaInitStruct_.GDMA_ReloadSrc = 1;
        rxDmaInitStruct_.GDMA_SrcHandshakeInterface = SPI_DEV_TABLE[rtlSpiIndex_].Rx_HandshakeInterface;
        rxDmaInitStruct_.GDMA_SrcAddr = (u32)&SPI_DEV_TABLE[rtlSpiIndex_].SPIx->DR;
        rxDmaInitStruct_.GDMA_SrcInc = NoChange;
        rxDmaInitStruct_.GDMA_SrcMsize = MsizeFour;
        rxDmaInitStruct_.GDMA_SrcDataWidth = TrWidthOneByte;
        rxDmaInitStruct_.GDMA_Index = 0;
        rxDmaInitStruct_.GDMA_ChNum = rxGdmaChannel;
        rxDmaInitStruct_.GDMA_IsrType = (BlockType|TransferType|ErrType);

        NVIC_SetPriority(GDMA_GetIrqNum(0, txDmaInitStruct_.GDMA_ChNum), CFG_GDMA_TX_PRIORITY);
        NVIC_SetPriority(GDMA_GetIrqNum(0, rxDmaInitStruct_.GDMA_ChNum), CFG_GDMA_RX_PRIORITY);

        return SYSTEM_ERROR_NONE;
    }

public:
    hal_spi_interface_t     spiInterface_;
    int                     rtlSpiIndex_;
    uint32_t                spiInputClock_;
    int                     prio_;
    hal_pin_t               csPin_;
    hal_pin_t               sclkPin_;
    hal_pin_t               mosiPin_;
    hal_pin_t               misoPin_;

    SpiConfig               config_;
    SpiTransferConfig       transferConfig_;
    SpiCallbackConfig       callbackConfig_;

    volatile uint8_t        ssPinState_;

    GDMA_InitTypeDef        rxDmaInitStruct_;
    GDMA_InitTypeDef        txDmaInitStruct_;
    os_mutex_recursive_t    mutex_;
    SpiStatus               status_;
};

Spi* getInstance(hal_spi_interface_t spi) {
    static Spi spiMap[] = {
#if PLATFORM_ID == PLATFORM_MSOM
        {HAL_SPI_INTERFACE1, 0, 100*1000*1000, CFG_SPI_PRIORITY, SS, SCK, MOSI, MISO},
        {HAL_SPI_INTERFACE2, 1, 50*1000*1000, CFG_SPI_PRIORITY, SS1, SCK1, MOSI1, MISO1}
#else
        {HAL_SPI_INTERFACE1, 1, 50*1000*1000, CFG_SPI_PRIORITY, SS, SCK, MOSI, MISO},
        {HAL_SPI_INTERFACE2, 0, 100*1000*1000, CFG_SPI_PRIORITY, SS1, SCK1, MOSI1, MISO1}
#endif // PLATFORM_ID == PLATFORM_MSOM
    };

    CHECK_TRUE(spi < sizeof(spiMap) / sizeof(spiMap[0]), nullptr);

    return &spiMap[spi];
}

} // Anonymous

void hal_spi_init(hal_spi_interface_t spi) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return;
    }
    auto spiInstance = getInstance(spi);
    spiInstance->init();
}

void hal_spi_begin(hal_spi_interface_t spi, uint16_t pin) {
    // Default to Master mode
    hal_spi_begin_ext(spi, SPI_MODE_MASTER, pin, nullptr);
}

void hal_spi_begin_ext(hal_spi_interface_t spi, hal_spi_mode_t mode, uint16_t pin, const hal_spi_config_t* spi_config) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return;
    }
    auto spiInstance = getInstance(spi);
    SpiConfig config = spiInstance->config();
    config.spiMode = mode;
    spiInstance->begin(pin, config, spi_config);
}

void hal_spi_end(hal_spi_interface_t spi) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return;
    }
    auto spiInstance = getInstance(spi);
    spiInstance->end();
}

void hal_spi_set_bit_order(hal_spi_interface_t spi, uint8_t order) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return;
    }
    getInstance(spi)->setBitOrder(order);
}

void hal_spi_set_data_mode(hal_spi_interface_t spi, uint8_t mode) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return;
    }
    getInstance(spi)->setDataMode(mode);
}

void hal_spi_set_clock_divider(hal_spi_interface_t spi, uint8_t rate) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return;
    }
    getInstance(spi)->setClockDivider(rate);
}

uint16_t hal_spi_transfer(hal_spi_interface_t spi, uint16_t data) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return 0;
    }
    auto spiInstance = getInstance(spi);
    auto config = spiInstance->config();
    if (config.spiMode == SPI_MODE_SLAVE) {
        return 0;
    }
    return spiInstance->transfer((uint8_t)data);
}

bool hal_spi_is_enabled(hal_spi_interface_t spi) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return false;
    }
    auto spiInstance = getInstance(spi);
    return spiInstance->isEnabled();
}

bool hal_spi_is_enabled_deprecated(void) {
    return false;
}

void hal_spi_info(hal_spi_interface_t spi, hal_spi_info_t* info, void* reserved) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return;
    }

    auto spiInstance = getInstance(spi);
    auto config = spiInstance->config();
    info->system_clock = spiInstance->getSpiInputClock();
    if (info->version >= HAL_SPI_INFO_VERSION_1) {
        int32_t state = HAL_disable_irq();
        if (spiInstance->isEnabled()) {
            //info->clock = info->system_clock / SPI_DEV_TABLE[spiInstance->rtlSpiIndex_].SPIx->BAUDR;
            switch (config.clockDiv) {
                case SPI_CLOCK_DIV2:   info->clock = info->system_clock / 2;   break;
                case SPI_CLOCK_DIV4:   info->clock = info->system_clock / 4;   break;
                case SPI_CLOCK_DIV8:   info->clock = info->system_clock / 8;   break;
                case SPI_CLOCK_DIV16:  info->clock = info->system_clock / 16;  break;
                case SPI_CLOCK_DIV32:  info->clock = info->system_clock / 32;  break;
                case SPI_CLOCK_DIV64:  info->clock = info->system_clock / 64;  break;
                case SPI_CLOCK_DIV128: info->clock = info->system_clock / 128; break;
                case SPI_CLOCK_DIV256: info->clock = info->system_clock / 256; break;
                default: info->clock = 0;
            }
        } else {
            info->clock = 0;
        }
        info->default_settings = ((config.spiMode  == DEFAULT_SPI_MODE) &&
                                  (config.bitOrder == DEFAULT_BIT_ORDER) &&
                                  (config.dataMode == DEFAULT_DATA_MODE) &&
                                  (config.clockDiv == DEFAULT_SPI_CLOCKDIV));
        info->enabled   = (spiInstance->isEnabled());
        info->mode      = config.spiMode;
        info->bit_order = config.bitOrder;
        info->data_mode = config.dataMode;
        if (info->version >= HAL_SPI_INFO_VERSION_2) {
            info->ss_pin = spiInstance->csPin();
        }
        HAL_enable_irq(state);
    }
}

void hal_spi_set_callback_on_selected(hal_spi_interface_t spi, hal_spi_select_user_callback cb, void* reserved) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return;
    }
    auto spiInstance = getInstance(spi);
    spiInstance->registerSelectUserCb(cb);
}

void hal_spi_transfer_dma(hal_spi_interface_t spi, const void* tx_buffer, void* rx_buffer, uint32_t length, hal_spi_dma_user_callback userCallback) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return;
    }
    auto spiInstance = getInstance(spi);
    spiInstance->transferDma((const uint8_t*)tx_buffer, (uint8_t*)rx_buffer, length, userCallback);
}

void hal_spi_transfer_dma_cancel(hal_spi_interface_t spi) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return;
    }
    auto spiInstance = getInstance(spi);
    spiInstance->transferDmaCancel(/* flushTx */ true);
}

int32_t hal_spi_transfer_dma_status(hal_spi_interface_t spi, hal_spi_transfer_status_t* st) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    auto spiInstance = getInstance(spi);
    auto status = spiInstance->status();
    int transferLength = spiInstance->isBusy() ? 0 : status.transferredLength;

    if (st) {
        st->ss_state = status.csPinSelected;
        st->transfer_ongoing = spiInstance->isBusy();
        st->configured_transfer_length = status.configuredTransferLength;
        st->transfer_length = transferLength;
    }
    return transferLength;
}

int32_t hal_spi_set_settings(hal_spi_interface_t spi, uint8_t set_default, uint8_t clockdiv, uint8_t order, uint8_t mode, void* reserved) {
    SpiConfig config = {};
    if (set_default) {
        config.dataMode = DEFAULT_DATA_MODE;
        config.bitOrder = DEFAULT_BIT_ORDER;
        config.clockDiv = DEFAULT_SPI_CLOCKDIV;
    } else {
        config.dataMode = mode;
        config.bitOrder = order;
        config.clockDiv = clockdiv;
    }

    auto spiInstance = CHECK_TRUE_RETURN(getInstance(spi), SYSTEM_ERROR_NOT_FOUND);
    // FIXME: performing end()/begin() is pretty slow due to GPIO ROM calls
    // Instead opt to change just the settings
    spiInstance->setSettings(config);
    return 0;
}

int hal_spi_sleep(hal_spi_interface_t spi, bool sleep, void* reserved) {
    auto spiInstance = CHECK_TRUE_RETURN(getInstance(spi), SYSTEM_ERROR_NOT_FOUND);
    if (sleep) {
        CHECK_TRUE(hal_spi_is_enabled(spi), SYSTEM_ERROR_INVALID_STATE);
        while (spiInstance->isBusy());
        spiInstance->end();
    } else  {
        CHECK_TRUE(hal_spi_is_enabled(spi) == false, SYSTEM_ERROR_INVALID_STATE);
        spiInstance->begin(spiInstance->config());
    }
    return SYSTEM_ERROR_NONE;
}

int32_t hal_spi_acquire(hal_spi_interface_t spi, const hal_spi_acquire_config_t* conf) {
    auto spiInstance = CHECK_TRUE_RETURN(getInstance(spi), SYSTEM_ERROR_NOT_FOUND);
    if (conf && conf->timeout == 0) {
        return spiInstance->lock(0);
    }
    return spiInstance->lock();
}

int32_t hal_spi_release(hal_spi_interface_t spi, void* reserved) {
    auto spiInstance = CHECK_TRUE_RETURN(getInstance(spi), SYSTEM_ERROR_NOT_FOUND);
    return spiInstance->unlock();
}

int hal_spi_get_clock_divider(hal_spi_interface_t spi, uint32_t clock, void* reserved) {
    CHECK_TRUE(clock > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
    auto spiInstance = CHECK_TRUE_RETURN(getInstance(spi), SYSTEM_ERROR_NOT_FOUND);
    auto spiInputClock = spiInstance->getSpiInputClock();
    // "clock" should be less or equal to half of the SPI input clock.
    CHECK_TRUE(clock <= spiInputClock, SYSTEM_ERROR_INVALID_ARGUMENT);

    // For clock rates which are not fractions of base clock use next power of 2 divisor
    // to be under the requested clock rate
    uint32_t div = spiInputClock / clock;
    div = div == 1 ? 1 : 1 << (std::numeric_limits<uint32_t>::digits - __builtin_clz(div - 1));

    switch (div) {
    case 2:
        return SPI_CLOCK_DIV2;
    case 4:
        return SPI_CLOCK_DIV4;
    case 8:
        return SPI_CLOCK_DIV8;
    case 16:
        return SPI_CLOCK_DIV16;
    case 32:
        return SPI_CLOCK_DIV32;
    case 64:
        return SPI_CLOCK_DIV64;
    case 128:
        return SPI_CLOCK_DIV128;
    case 256:
    default:
        return SPI_CLOCK_DIV256;
    }

    return SYSTEM_ERROR_UNKNOWN;
}

