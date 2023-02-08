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
extern "C" {
#include "rtl8721d.h"
extern void SSI_SetDataSwap(SPI_TypeDef *spi_dev, u32 SwapStatus, u32 newState);
// #include "rtl8721d_ssi.h"
// #include "rtl8721dhp_rcc.h"
}

#include <memory>
#include <cstdlib>

#define DEFAULT_SPI_MODE        SPI_MODE_MASTER
#define DEFAULT_DATA_MODE       SPI_MODE3
#define DEFAULT_DATA_BITS       DFS_8_BITS
#define DEFAULT_BIT_ORDER       MSBFIRST
#define DEFAULT_SPI_CLOCKDIV    SPI_CLOCK_DIV256

#define CFG_SPI_PRIORITY        6
#define CFG_GDMA_TX_PRIORITY    4
#define CFG_GDMA_RX_PRIORITY    3
#define CFG_CHUNK_BUF_SIZE      32

static_assert((CFG_CHUNK_BUF_SIZE&0x1f) == 0, "Chunk size should be multiple of 32-byte");

namespace {

// Copied from spark_wiring_interrupts.h
// Ideally this should be moved into services. HAL shouldn't depend on wiring
class AtomicSection {
    int prev_;
public:
    AtomicSection() {
        prev_ = HAL_disable_irq();
    }

    ~AtomicSection() {
        HAL_enable_irq(prev_);
    }
};

class Spi;
Spi* getInstance(hal_spi_interface_t spi);

typedef struct {
    hal_spi_mode_t  spiMode;
    uint8_t         dataMode;
    uint8_t         bitOrder;
    uint32_t        clockDiv;
    void reset() {
        spiMode  = DEFAULT_SPI_MODE;
        dataMode = DEFAULT_DATA_MODE;
        bitOrder = DEFAULT_BIT_ORDER;
        clockDiv = DEFAULT_SPI_CLOCKDIV;
    }
} SpiConfig;

typedef struct {
    volatile const uint8_t*     txBuf;
    volatile uint8_t*           rxBuf;
    size_t                      txLength;
    size_t                      rxLength;
    void reset() {
        txBuf  = nullptr;
        rxBuf  = nullptr;
        txLength = 0;
        rxLength = 0;
    }
} SpiBufferConfig;

typedef struct {
    volatile __attribute__((aligned(32))) uint8_t    txBuf[CFG_CHUNK_BUF_SIZE];
    volatile __attribute__((aligned(32))) uint8_t    rxBuf[CFG_CHUNK_BUF_SIZE];
    volatile size_t                                  txLength;
    volatile size_t                                  rxLength;
    volatile size_t                                  txIndex;
    volatile size_t                                  rxIndex;
} SpiChunkBufferConfig;

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
    volatile bool               userDmaCbHandled;
    volatile uint16_t           transferredLength;
    volatile uint16_t           configuredTransferLength;
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
              bufferConfig_{},
              callbackConfig_{},
              ssPinState_{},
              rxDmaInitStruct_{},
              txDmaInitStruct_{},
              mutex_{},
              status_{} {
        config_.reset();
    }

    int init() {
        AtomicSection lk;

        if (isEnabled()) {
            CHECK(end());
        }
        if (mutex_ == nullptr) {
            os_mutex_recursive_create(&mutex_);
        }
        config_.reset();
        bufferConfig_.reset();
        return SYSTEM_ERROR_NONE;
    }

    int deinit() {
        end();
        os_mutex_recursive_destroy(&mutex_);
        return SYSTEM_ERROR_NONE;
    }

    int begin(const SpiConfig& config) {
        return begin(csPin_, config, nullptr);
    }

    int begin(uint16_t csPin, const SpiConfig& config, const hal_spi_config_t* spi_config) {
        AtomicSection lk;

        // Start address of chunk buffer should be 32-byte aligned
        SPARK_ASSERT(((uint32_t)chunkBuffer_.txBuf & 0x1f) == 0);
        SPARK_ASSERT(((uint32_t)chunkBuffer_.rxBuf & 0x1f) == 0);
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
        csPin_ = csPin;

        if (isEnabled()) {
            CHECK(end());
        }

        // Allocate dam channels
        CHECK(initDmaChannels());

        // Save config
        memcpy(&config_, &config, sizeof(SpiConfig));

        SSI_InitTypeDef SSI_InitStruct;
        SSI_StructInit(&SSI_InitStruct);

        // Enable SPI Clock
        if (rtlSpiIndex_ == 0) {
            RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);
        } else {
            RCC_PeriphClockCmd(APBPeriph_SPI1, APBPeriph_SPI1_CLOCK, ENABLE);
        }

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

        hal_gpio_set_drive_strength(sclkPin_, HAL_GPIO_DRIVE_HIGH);
        hal_gpio_set_drive_strength(mosiPin_, HAL_GPIO_DRIVE_HIGH);
        hal_gpio_set_drive_strength(misoPin_, HAL_GPIO_DRIVE_HIGH);

        uint32_t phase = SCPH_TOGGLES_IN_MIDDLE;
        uint32_t polarity = SCPOL_INACTIVE_IS_LOW;
        spiModeToPolAndPha(config_.dataMode, &polarity, &phase);
        SSI_SetSclkPhase(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, phase);
        SSI_SetSclkPolarity(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, polarity);
        SSI_SetDataFrameSize(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, DEFAULT_DATA_BITS);
        if (polarity == SCPOL_INACTIVE_IS_LOW) {
            PAD_PullCtrl((uint8_t)hal_pin_to_rtl_pin(sclkPin_), GPIO_PuPd_DOWN);
        } else {
            PAD_PullCtrl((uint8_t)hal_pin_to_rtl_pin(sclkPin_), GPIO_PuPd_UP);
        }

        // Set pin function
        if (!(spi_config && (spi_config->flags & (uint32_t)HAL_SPI_CONFIG_FLAG_MOSI_ONLY))) {
            hal_pin_set_function(sclkPin_, PF_SPI);
            hal_pin_set_function(misoPin_, PF_SPI);
        }
        hal_pin_set_function(mosiPin_, PF_SPI);

        // Set clock divider
        if (config_.spiMode == SPI_MODE_MASTER) {
            u32 rtlClockDivider = 256;
            clockDivToRtlClockDiv(config_.clockDiv, &rtlClockDivider);
            SSI_SetBaudDiv(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, rtlClockDivider);

            // Set sample delay for SPI0@50MHz
            if (rtlClockDivider == 2 && SPI_DEV_TABLE[rtlSpiIndex_].SPIx == SPI0_DEV) {
                SSI_SetSampleDelay(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, 0x1);
            } else {
                SSI_SetSampleDelay(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, 0x0);
            }
        }

        // Set bit order
        if (config_.bitOrder == MSBFIRST) {
            SSI_SetDataSwap(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, BIT_CTRLR0_TXBITSWP, DISABLE);
            SSI_SetDataSwap(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, BIT_CTRLR0_RXBITSWP, DISABLE);
        } else {
            SSI_SetDataSwap(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, BIT_CTRLR0_TXBITSWP, ENABLE);
            SSI_SetDataSwap(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, BIT_CTRLR0_RXBITSWP, ENABLE);
        }

        // Enable SPI DMA
        SSI_SetDmaEnable(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, ENABLE, BIT_SHIFT_DMACR_TDMAE);
        SSI_SetDmaEnable(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, ENABLE, BIT_SHIFT_DMACR_RDMAE);

        // We don't use SPI interrupt
        SSI_INTConfig(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, (BIT_IMR_RXFIM | BIT_IMR_RXOIM | BIT_IMR_RXUIM), DISABLE);
        SSI_INTConfig(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, (BIT_IMR_TXOIM | BIT_IMR_TXEIM), DISABLE);
        InterruptRegister(interruptHandler, SPI_DEV_TABLE[rtlSpiIndex_].IrqNum, (u32)this, prio_);
        NVIC_ClearPendingIRQ(SPI_DEV_TABLE[rtlSpiIndex_].IrqNum);
        InterruptDis(SPI_DEV_TABLE[rtlSpiIndex_].IrqNum);

        // Clear dma interrupt
        GDMA_ClearINT(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum);
        GDMA_ClearINT(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum);

        // Update state
        status_.state = HAL_SPI_STATE_ENABLED;

        //LOG_DEBUG(INFO, "SPI begin! mode: %d, order: %s",config_.dataMode, config_.bitOrder ? "MSB" : "LSB");

        return SYSTEM_ERROR_NONE;
    }

    int end() {
        CHECK_TRUE(status_.state == HAL_SPI_STATE_ENABLED, SYSTEM_ERROR_INVALID_STATE);

        // Wait for last SPI transfer finished
        while (isBusy()) {
            ;
        }

        AtomicSection lk;

        /* Set SSI DMA Disable */
        SSI_SetDmaEnable(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, DISABLE, BIT_SHIFT_DMACR_RDMAE);
        SSI_SetDmaEnable(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, DISABLE, BIT_SHIFT_DMACR_TDMAE);

        GDMA_ChCleanAutoReload(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, CLEAN_RELOAD_SRC_DST);
        GDMA_ChCleanAutoReload(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, CLEAN_RELOAD_SRC_DST);

        /* Clear Pending ISR */
        GDMA_ClearINT(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum);
        GDMA_ClearINT(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum);
        GDMA_ChnlFree(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum);
        GDMA_ChnlFree(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum);

        GDMA_Cmd(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, DISABLE);
        GDMA_Cmd(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, DISABLE);

        // Set GPIO, pin function and pinmux
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

        // Update state
        status_.state = HAL_SPI_STATE_DISABLED;
        status_.transferredLength = 0;

        return SYSTEM_ERROR_NONE;
    }

    int lock(system_tick_t timeout = 0) {
        CHECK_TRUE(hal_interrupt_is_isr() == false, SYSTEM_ERROR_INVALID_STATE);
        // FIXME: os_mutex_recursive_lock doesn't take any arguments, using trylock for now
        if (timeout) {
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
        CHECK_TRUE(isBusy() == false, 0);
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

    void startTransmission() {
        AtomicSection lk;

        if (status_.transmitting) {
            return;
        }

        if (chunkBuffer_.txIndex >= bufferConfig_.txLength) {
            chunkBuffer_.txIndex = 0;
            return;
        }

        chunkBuffer_.txLength = std::min(bufferConfig_.txLength - chunkBuffer_.txIndex, (size_t)CFG_CHUNK_BUF_SIZE);
        if (bufferConfig_.txBuf) {
            memcpy((void*)chunkBuffer_.txBuf, (void*)&bufferConfig_.txBuf[chunkBuffer_.txIndex], chunkBuffer_.txLength);
        } else {
            memset((void*)chunkBuffer_.txBuf, 0xFF, chunkBuffer_.txLength);
        }
        DCache_CleanInvalidate((u32) chunkBuffer_.txBuf, chunkBuffer_.txLength);
        //LOG_DEBUG(INFO, "start to send new chunk, curr index: %d, length: %d", chunkBuffer_.txIndex, chunkBuffer_.txLength);
        if (((chunkBuffer_.txLength & 0x03)==0) && (((u32)(chunkBuffer_.txBuf) & 0x03)==0)) {
            /*  4-bytes aligned, move 4 bytes each transfer */
            txDmaInitStruct_.GDMA_SrcMsize   = MsizeOne;
            txDmaInitStruct_.GDMA_SrcDataWidth = TrWidthFourBytes;
            txDmaInitStruct_.GDMA_BlockSize = chunkBuffer_.txLength >> 2;
        } else {
            txDmaInitStruct_.GDMA_SrcMsize   = MsizeFour;
            txDmaInitStruct_.GDMA_SrcDataWidth = TrWidthOneByte;
            txDmaInitStruct_.GDMA_BlockSize = chunkBuffer_.txLength;
        }
        txDmaInitStruct_.GDMA_DstMsize  = MsizeFour;
        txDmaInitStruct_.GDMA_DstDataWidth =  TrWidthOneByte;
        assert_param(txDmaInitStruct_.GDMA_BlockSize <= 4096);
        txDmaInitStruct_.GDMA_SrcAddr = (u32)chunkBuffer_.txBuf;

        /*  Enable GDMA for TX */
        GDMA_Init(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, &txDmaInitStruct_);
        GDMA_Cmd(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, ENABLE);
        status_.transmitting = true;
    }

    void startReceiver() {
        AtomicSection lk;

        if (status_.receiving) {
            return;
        }

        // Update transferred data length
        status_.transferredLength = chunkBuffer_.rxIndex;

        if (chunkBuffer_.rxIndex >= bufferConfig_.rxLength) {
            chunkBuffer_.rxIndex = 0;

            if (config_.spiMode == SPI_MODE_MASTER) {
                // FIXME: For SPI slave, the user callback will be called after CS pin is pulled high.
                if (callbackConfig_.dmaUserCb) {
                    (*callbackConfig_.dmaUserCb)();
                }
                status_.userDmaCbHandled = true;
            }
            return;
        }

        if (config_.spiMode == SPI_MODE_MASTER) {
            chunkBuffer_.rxLength = std::min(bufferConfig_.rxLength - chunkBuffer_.rxIndex, (size_t)CFG_CHUNK_BUF_SIZE);
        } else {
            // FIXME: For SPI slave, the dma buffer cannot be less than 4 bytes, otherwise spi slave receives nothing
            //        when the master sends less than 4bytes data.
            //        If the user uses Non-four-byte alignment buffer, the user callback will be called after CS pin is pulled high.
            chunkBuffer_.rxLength = CFG_CHUNK_BUF_SIZE;
        }

        //  8~4 bits mode
        rxDmaInitStruct_.GDMA_SrcMsize = MsizeFour;
        rxDmaInitStruct_.GDMA_SrcDataWidth = TrWidthOneByte;
        rxDmaInitStruct_.GDMA_BlockSize = chunkBuffer_.rxLength;
        if (((chunkBuffer_.rxLength & 0x03)==0) && (((u32)(chunkBuffer_.rxBuf) & 0x03)==0)) {
            /*  4-bytes aligned, move 4 bytes each transfer */
            rxDmaInitStruct_.GDMA_DstMsize = MsizeOne;
            rxDmaInitStruct_.GDMA_DstDataWidth = TrWidthFourBytes;
        } else {
            rxDmaInitStruct_.GDMA_DstMsize = MsizeFour;
            rxDmaInitStruct_.GDMA_DstDataWidth = TrWidthOneByte;
        }
        assert_param(rxDmaInitStruct_.GDMA_BlockSize <= 4096);
        rxDmaInitStruct_.GDMA_DstAddr = (u32)chunkBuffer_.rxBuf;

        /*  Enable GDMA for RX */
        GDMA_Init(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, &rxDmaInitStruct_);
        GDMA_Cmd(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, ENABLE);
        status_.receiving = true;
    }

    int stopTransfer() {
        AtomicSection lk;

        if (!status_.transmitting && !status_.receiving) {
            return SYSTEM_ERROR_INVALID_STATE;
        }

        if (status_.receiving) {
            uint32_t dmaStopAddress = GDMA_GetDstAddr(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum);
            size_t dmaRxCount = dmaStopAddress ? (dmaStopAddress - (uint32_t)chunkBuffer_.rxBuf) : 0;
            size_t fifoRxCount = 0;
            size_t bytesToCopy = 0;
            if (dmaRxCount < chunkBuffer_.rxLength) {
                fifoRxCount = SSI_ReceiveData(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, (void*)&chunkBuffer_.rxBuf[dmaRxCount], chunkBuffer_.rxLength - dmaRxCount);
                bytesToCopy = std::min(bufferConfig_.rxLength - chunkBuffer_.rxIndex, dmaRxCount + fifoRxCount);
                if (bufferConfig_.rxBuf) {
                    DCache_Invalidate((u32) chunkBuffer_.rxBuf, chunkBuffer_.rxLength);
                    int primask = HAL_disable_irq();
                    memcpy((void*)&bufferConfig_.rxBuf[chunkBuffer_.rxIndex], (void*)chunkBuffer_.rxBuf, bytesToCopy);
                    DCache_CleanInvalidate((u32) bufferConfig_.rxBuf, bufferConfig_.rxLength);
                    HAL_enable_irq(primask);
                }
                chunkBuffer_.rxIndex += bytesToCopy;

            } else {
                LOG_DEBUG(ERROR, "dmaStopAddress is not reliable.");
            }

            status_.transferredLength = chunkBuffer_.rxIndex;
        }

        // TX data has been moved to DMA peripheral, force to clean it
        GDMA_ChCleanAutoReload(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, CLEAN_RELOAD_SRC_DST);
        GDMA_ClearINT(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum);
        GDMA_Cmd(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, DISABLE);
        status_.transmitting = false;
        chunkBuffer_.txIndex = 0;

        GDMA_ChCleanAutoReload(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, CLEAN_RELOAD_SRC_DST);
        GDMA_ClearINT(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum);
        GDMA_Cmd(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, DISABLE);
        status_.receiving = false;
        chunkBuffer_.rxIndex = 0;

        return SYSTEM_ERROR_NONE;
    }

    int transferDma(const uint8_t* txBuf, uint8_t* rxBuf, size_t size, hal_spi_dma_user_callback callback) {
        CHECK_TRUE((txBuf || rxBuf) && size > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(isEnabled(), SYSTEM_ERROR_INVALID_STATE);

        // Wait for last SPI master transfer finished
        while ((config_.spiMode == SPI_MODE_MASTER) && isBusy()) {
            ;
        }

        AtomicSection lk;

        // Reset SPI in case the data in the buffer is not read completely in the last transfer
        transferDmaCancel();
        SSI_Cmd(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, DISABLE);
        SSI_Cmd(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, ENABLE);

        bufferConfig_.rxBuf = rxBuf;
        bufferConfig_.rxLength = size;
        bufferConfig_.txBuf = txBuf;
        bufferConfig_.txLength = size;

        callbackConfig_.dmaUserCb = callback;
        status_.configuredTransferLength = size;
        status_.transferredLength = 0;

        chunkBuffer_.txIndex = 0;
        chunkBuffer_.rxIndex = 0;

        startTransmission();
        startReceiver();

        return SYSTEM_ERROR_NONE;
    }

    int transferDmaCancel() {
        AtomicSection lk;

        /* Clear Pending ISR */
        GDMA_ChCleanAutoReload(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, CLEAN_RELOAD_SRC_DST);
        GDMA_ChCleanAutoReload(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, CLEAN_RELOAD_SRC_DST);
        GDMA_Cmd(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, DISABLE);
        GDMA_Cmd(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, DISABLE);
        GDMA_ClearINT(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum);
        GDMA_ClearINT(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum);

        status_.transmitting = false;
        status_.receiving = false;
        callbackConfig_.dmaUserCb = nullptr;

        // Flush fifo
        u32 rxFifoLevel;
        while (SSI_Readable(SPI_DEV_TABLE[rtlSpiIndex_].SPIx)) {
            rxFifoLevel = SSI_GetRxCount(SPI_DEV_TABLE[rtlSpiIndex_].SPIx);
            for (u32 i = 0; i < rxFifoLevel; i++) {
                SSI_ReadData(SPI_DEV_TABLE[rtlSpiIndex_].SPIx);
            }
        }

        return SYSTEM_ERROR_NONE;
    }

    SpiConfig config() const {
        return config_;
    }

    int setConfig(SpiConfig& config) {
        CHECK_TRUE(validateConfig(rtlSpiIndex_, config), SYSTEM_ERROR_INVALID_ARGUMENT);
        memcpy(&config_, &config, sizeof(SpiConfig));
        return SYSTEM_ERROR_NONE;
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

    bool isBusy() const {
        return status_.transmitting || status_.receiving;
    }

    bool isSuspended() const {
        return status_.state == HAL_SPI_STATE_SUSPENDED;
    }

    bool isDmaBufferConfigured() const {
        // Use at least one buffer for SPI transfer
        return bufferConfig_.txBuf || bufferConfig_.rxBuf;
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

    void dmaTxHandlerImpl() {
        // Clear Pending ISR, free TX DMA resource
        GDMA_ChCleanAutoReload(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, CLEAN_RELOAD_SRC_DST);
        GDMA_Cmd(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, DISABLE);
        uint32_t isrTypeMap = GDMA_ClearINT(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum);
        (void)isrTypeMap;
        status_.transmitting = false;

        chunkBuffer_.txIndex += chunkBuffer_.txLength;

        startTransmission();
    }

    void dmaRxHandlerImpl() {
        // Clear Pending ISR, free RX DMA resource
        GDMA_ChCleanAutoReload(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, CLEAN_RELOAD_SRC_DST);
        GDMA_Cmd(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, DISABLE);
        GDMA_ClearINT(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum);
        status_.receiving = false;

        /// Transfer in progress
        uint32_t copyLength = 0;
        copyLength = std::min(bufferConfig_.rxLength - chunkBuffer_.rxIndex, (size_t)chunkBuffer_.rxLength);
        if (bufferConfig_.rxBuf) {
            DCache_Invalidate((u32) chunkBuffer_.rxBuf, chunkBuffer_.rxLength);
            int primask = HAL_disable_irq();
            memcpy((void*)&bufferConfig_.rxBuf[chunkBuffer_.rxIndex], (void*)chunkBuffer_.rxBuf, copyLength);
            DCache_CleanInvalidate((u32) bufferConfig_.rxBuf, bufferConfig_.rxLength);
            HAL_enable_irq(primask);
        }
        chunkBuffer_.rxIndex += copyLength;

        startReceiver();
    }

    void onSelectedHandlerImpl() {
        status_.csPinSelected = !hal_gpio_read(csPin_);
        if (callbackConfig_.selectUserCb) {
            (*callbackConfig_.selectUserCb)(status_.csPinSelected);
        }

        if (isDmaBufferConfigured()) {
            if (status_.csPinSelected) {
                status_.userDmaCbHandled = false;
                SSI_Cmd(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, ENABLE);

                // Releaod DMA buffer
                startTransmission();
                startReceiver();
            } else {
                stopTransfer();
                SSI_Cmd(SPI_DEV_TABLE[rtlSpiIndex_].SPIx, DISABLE);
                if (status_.userDmaCbHandled == false) {
                    // SPI master sends the data that is less than the buffer size in the SPI slave
                    if (callbackConfig_.dmaUserCb) {
                        (*callbackConfig_.dmaUserCb)();
                    }
                }
            }
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

        _memset(&txDmaInitStruct_, 0, sizeof(GDMA_InitTypeDef));
        txDmaInitStruct_.GDMA_DIR = TTFCMemToPeri;
        txDmaInitStruct_.GDMA_DstHandshakeInterface = SPI_DEV_TABLE[rtlSpiIndex_].Tx_HandshakeInterface;
        txDmaInitStruct_.GDMA_DstAddr = (u32)&SPI_DEV_TABLE[rtlSpiIndex_].SPIx->DR;
        txDmaInitStruct_.GDMA_Index = 0;
        txDmaInitStruct_.GDMA_ChNum = txGdmaChannel;
        txDmaInitStruct_.GDMA_IsrType = (BlockType|TransferType|ErrType);
        txDmaInitStruct_.GDMA_SrcMsize = MsizeOne;
        txDmaInitStruct_.GDMA_DstMsize = MsizeFour;
        txDmaInitStruct_.GDMA_SrcDataWidth = TrWidthFourBytes;
        txDmaInitStruct_.GDMA_DstDataWidth = TrWidthOneByte;
        txDmaInitStruct_.GDMA_DstInc = NoChange;
        txDmaInitStruct_.GDMA_SrcInc = IncType;

        _memset(&rxDmaInitStruct_, 0, sizeof(GDMA_InitTypeDef));
        rxDmaInitStruct_.GDMA_DIR = TTFCPeriToMem;
        rxDmaInitStruct_.GDMA_ReloadSrc = 0;
        rxDmaInitStruct_.GDMA_SrcHandshakeInterface = SPI_DEV_TABLE[rtlSpiIndex_].Rx_HandshakeInterface;
        rxDmaInitStruct_.GDMA_SrcAddr = (u32)&SPI_DEV_TABLE[rtlSpiIndex_].SPIx->DR;
        rxDmaInitStruct_.GDMA_Index = 0;
        rxDmaInitStruct_.GDMA_ChNum = rxGdmaChannel;
        rxDmaInitStruct_.GDMA_IsrType = (BlockType|TransferType|ErrType);
        rxDmaInitStruct_.GDMA_SrcMsize = MsizeEight;
        rxDmaInitStruct_.GDMA_DstMsize = MsizeFour;
        rxDmaInitStruct_.GDMA_SrcDataWidth = TrWidthTwoBytes;
        rxDmaInitStruct_.GDMA_DstDataWidth = TrWidthFourBytes;
        rxDmaInitStruct_.GDMA_DstInc = IncType;
        rxDmaInitStruct_.GDMA_SrcInc = NoChange;

        NVIC_SetPriority(GDMA_GetIrqNum(0, txDmaInitStruct_.GDMA_ChNum), CFG_GDMA_TX_PRIORITY);
        NVIC_SetPriority(GDMA_GetIrqNum(0, rxDmaInitStruct_.GDMA_ChNum), CFG_GDMA_RX_PRIORITY);

        return SYSTEM_ERROR_NONE;
    }

private:
    hal_spi_interface_t     spiInterface_;
    int                     rtlSpiIndex_;
    uint32_t                spiInputClock_;
    int                     prio_;
    hal_pin_t               csPin_;
    hal_pin_t               sclkPin_;
    hal_pin_t               mosiPin_;
    hal_pin_t               misoPin_;

    SpiConfig               config_;
    SpiBufferConfig         bufferConfig_;
    SpiCallbackConfig       callbackConfig_;

    volatile uint8_t        ssPinState_;

    GDMA_InitTypeDef        rxDmaInitStruct_;
    GDMA_InitTypeDef        txDmaInitStruct_;
    os_mutex_recursive_t    mutex_;
    SpiStatus               status_;

    SpiChunkBufferConfig    chunkBuffer_;

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
    auto spiInstance = getInstance(spi);
    auto config = spiInstance->config();
    config.bitOrder = order;
    spiInstance->setConfig(config);
    if (spiInstance->isEnabled()) {
        spiInstance->begin(config);
    }
}

void hal_spi_set_data_mode(hal_spi_interface_t spi, uint8_t mode) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return;
    }
    auto spiInstance = getInstance(spi);
    auto config = spiInstance->config();
    config.dataMode = mode;
    spiInstance->setConfig(config);
    if (spiInstance->isEnabled()) {
        spiInstance->begin(config);
    }
}

void hal_spi_set_clock_divider(hal_spi_interface_t spi, uint8_t rate) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return;
    }
    auto spiInstance = getInstance(spi);
    auto config = spiInstance->config();
    config.clockDiv = rate;
    spiInstance->setConfig(config);
    if (spiInstance->isEnabled()) {
        spiInstance->begin(config);
    }
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
    spiInstance->transferDmaCancel();
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
    if (spiInstance->isEnabled()) {
        spiInstance->begin(config);
    }
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

    // Integer division results in clean values
    switch (spiInputClock / clock) {
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

