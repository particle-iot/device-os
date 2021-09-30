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
#define CFG_GDMA_TX_PRIORITY    10
#define CFG_GDMA_RX_PRIORITY    11

// #pragma GCC optimize 0 //FIXME: Remove it

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
    const uint8_t*              txBuf;
    uint8_t*                    rxBuf;
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
    hal_spi_dma_user_callback       dmaUserCb;
    hal_spi_select_user_callback    selectUserCb;
    void reset() {
        dmaUserCb = nullptr;
        selectUserCb = nullptr;
    }
} SpiCallbackConfig;

typedef struct {
    volatile hal_spi_state_t    state;
    volatile bool               dmaMode;
    volatile bool               rxOngoing;
    volatile bool               txOngoing;
    volatile bool               csPinSelected;
    volatile uint16_t           transferLength;
} SpiStatus;

class Spi {
public:
    Spi(const hal_spi_interface_t spiIndex, int prio,
            hal_pin_t csPin, hal_pin_t clkPin, hal_pin_t mosiPin, hal_pin_t misoPin)
            : spiInterface_(spiIndex),
              prio_(prio),
              csPin_(csPin),
              sclkPin_(clkPin),
              mosiPin_(mosiPin),
              misoPin_(misoPin),
              config_{},
              bufferConfig_{},
              callbackConfig_{},
              ssPinState_{},
              rxGdma_{},
              txGdma_{},
              mutex_{},
              status_{} {
        config_.reset();
    }

    int init() {
        if (isEnabled()) {
            CHECK(end());
        }
        AtomicSection lk;
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
        LOG(INFO, "spi begin()...");
        return begin(csPin_, config);
    }

    int begin(uint16_t csPin, const SpiConfig& config) {
        LOG(INFO, "spi begin(...)...");
        CHECK_TRUE(validateConfig(spiInterface_, config), SYSTEM_ERROR_INVALID_ARGUMENT);

        AtomicSection lk;
        if (isEnabled()) {
            CHECK(end());
        }

        // Save config
        csPin_ = csPin;
        memcpy(&config_, &config, sizeof(SpiConfig));

        SSI_InitTypeDef SSI_InitStruct;
        SSI_StructInit(&SSI_InitStruct);

        // Enable SPI Clock
        if (spiInterface_ == HAL_SPI_INTERFACE1) {
            RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);
        } else {
            RCC_PeriphClockCmd(APBPeriph_SPI1, APBPeriph_SPI1_CLOCK, ENABLE);
        }

        // Configure GPIO and Role
        if (config_.spiMode == SPI_MODE_MASTER) {
            SSI_InitStruct.SPI_Role = SSI_MASTER;
            SSI_SetRole(SPI_DEV_TABLE[spiInterface_].SPIx, SSI_MASTER);
            Pinmux_Config((uint8_t)hal_pin_to_rtl_pin(sclkPin_), PINMUX_FUNCTION_SPIM);
            Pinmux_Config((uint8_t)hal_pin_to_rtl_pin(mosiPin_), PINMUX_FUNCTION_SPIM);
            Pinmux_Config((uint8_t)hal_pin_to_rtl_pin(misoPin_), PINMUX_FUNCTION_SPIM);
            hal_gpio_write(csPin_, 1);
            hal_gpio_mode(csPin_, OUTPUT);
        } else {
            SSI_InitStruct.SPI_Role = SSI_SLAVE;
            SSI_SetRole(SPI_DEV_TABLE[spiInterface_].SPIx, SSI_SLAVE);
            Pinmux_Config((uint8_t)hal_pin_to_rtl_pin(sclkPin_), PINMUX_FUNCTION_SPIS);
            Pinmux_Config((uint8_t)hal_pin_to_rtl_pin(mosiPin_), PINMUX_FUNCTION_SPIS);
            Pinmux_Config((uint8_t)hal_pin_to_rtl_pin(misoPin_), PINMUX_FUNCTION_SPIS);
            hal_gpio_mode(csPin_, INPUT_PULLUP);
            hal_interrupt_attach(csPin_, &onSelectedHandler, this, CHANGE, nullptr);
        }
        SSI_Init(SPI_DEV_TABLE[spiInterface_].SPIx, &SSI_InitStruct);

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
        u32 SclkPhase;
        u32 SclkPolarity;
        switch (config_.dataMode) {
            case SPI_MODE0: {
                SclkPolarity = SCPOL_INACTIVE_IS_LOW;
                SclkPhase    = SCPH_TOGGLES_IN_MIDDLE;
                break;
            };
            case SPI_MODE1: {
                SclkPolarity = SCPOL_INACTIVE_IS_LOW;
                SclkPhase    = SCPH_TOGGLES_AT_START;
                break;
            };
            case SPI_MODE2: {
                SclkPolarity = SCPOL_INACTIVE_IS_HIGH;
                SclkPhase    = SCPH_TOGGLES_IN_MIDDLE;
                break;
            };
            case SPI_MODE3: {
                SclkPolarity = SCPOL_INACTIVE_IS_HIGH;
                SclkPhase    = SCPH_TOGGLES_AT_START;
                break;
            };
            default:
                // Shouldn't enter this case after parameter check
                SclkPolarity = SCPOL_INACTIVE_IS_HIGH;
                SclkPhase    = SCPH_TOGGLES_AT_START;
                break;
        }
        SSI_SetSclkPhase(SPI_DEV_TABLE[spiInterface_].SPIx, SclkPhase);
        SSI_SetSclkPolarity(SPI_DEV_TABLE[spiInterface_].SPIx, SclkPolarity);
        SSI_SetDataFrameSize(SPI_DEV_TABLE[spiInterface_].SPIx, DEFAULT_DATA_BITS);

        // Additional pin configuration based on the SPI mode
        if (config_.spiMode == SPI_MODE_MASTER) {
            if (SclkPolarity == SCPOL_INACTIVE_IS_LOW) {
                PAD_PullCtrl((uint8_t)hal_pin_to_rtl_pin(sclkPin_), GPIO_PuPd_DOWN);
            } else {
                PAD_PullCtrl((uint8_t)hal_pin_to_rtl_pin(sclkPin_), GPIO_PuPd_UP);
            }
        }

        // Set pin function
        hal_pin_set_function(sclkPin_, PF_SPI);
        hal_pin_set_function(mosiPin_, PF_SPI);
        hal_pin_set_function(misoPin_, PF_SPI);

        // Set clock divider
        u32 rtlClockDivider = 256;
        switch (config_.clockDiv) {
            case SPI_CLOCK_DIV2:   rtlClockDivider = 2;   break;
            case SPI_CLOCK_DIV4:   rtlClockDivider = 4;   break;
            case SPI_CLOCK_DIV8:   rtlClockDivider = 8;   break;
            case SPI_CLOCK_DIV16:  rtlClockDivider = 16;  break;
            case SPI_CLOCK_DIV32:  rtlClockDivider = 32;  break;
            case SPI_CLOCK_DIV64:  rtlClockDivider = 64;  break;
            case SPI_CLOCK_DIV128: rtlClockDivider = 128; break;
            case SPI_CLOCK_DIV256: rtlClockDivider = 256; break;
            default: rtlClockDivider = 256; break;
        }
        SSI_SetBaudDiv(SPI_DEV_TABLE[spiInterface_].SPIx, rtlClockDivider);

        // Configure interrupt
	    SSI_INTConfig(SPI_DEV_TABLE[spiInterface_].SPIx, (BIT_IMR_RXFIM | BIT_IMR_RXOIM | BIT_IMR_RXUIM), DISABLE);
        SSI_INTConfig(SPI_DEV_TABLE[spiInterface_].SPIx, (BIT_IMR_TXOIM | BIT_IMR_TXEIM), DISABLE);
        InterruptRegister(interruptHandler, SPI_DEV_TABLE[spiInterface_].IrqNum, (u32)this, prio_);
        NVIC_ClearPendingIRQ(SPI_DEV_TABLE[spiInterface_].IrqNum);
        InterruptEn(SPI_DEV_TABLE[spiInterface_].IrqNum, prio_);

        // Update state
        status_.state = HAL_SPI_STATE_ENABLED;

        return SYSTEM_ERROR_NONE;
    }

    int end() {
        CHECK_TRUE(status_.state == HAL_SPI_STATE_ENABLED, SYSTEM_ERROR_INVALID_STATE);

        // Disable SPI and interrupt
        InterruptDis(SPI_DEV_TABLE[spiInterface_].IrqNum);
        InterruptUnRegister(SPI_DEV_TABLE[spiInterface_].IrqNum);
	    SSI_INTConfig(SPI_DEV_TABLE[spiInterface_].SPIx, (BIT_IMR_RXFIM | BIT_IMR_RXOIM | BIT_IMR_RXUIM), DISABLE);
        SSI_INTConfig(SPI_DEV_TABLE[spiInterface_].SPIx, (BIT_IMR_TXOIM | BIT_IMR_TXEIM), DISABLE);
        SSI_Cmd(SPI_DEV_TABLE[spiInterface_].SPIx, DISABLE);

        // Set GPIO and pin function
        hal_gpio_mode(sclkPin_, INPUT_PULLUP);
        hal_gpio_mode(mosiPin_, PIN_MODE_NONE);
        hal_gpio_mode(misoPin_, PIN_MODE_NONE);
        hal_pin_set_function(sclkPin_, PF_NONE);
        hal_pin_set_function(mosiPin_, PF_NONE);
        hal_pin_set_function(misoPin_, PF_NONE);

        // Update state
        status_.state = HAL_SPI_STATE_DISABLED;
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

    int transfer(uint8_t* txBuf, uint8_t* rxBuf, size_t size) {
        CHECK_TRUE(isEnabled(), SYSTEM_ERROR_INVALID_STATE);
        CHECK_TRUE(config_.spiMode == SPI_MODE_MASTER, SYSTEM_ERROR_INVALID_STATE);

        // Wait for last SPI transfer finished
        while (isBusy()) {
            ;
        }

        // buffer and length will be changed during transfer to send the large packet
        bufferConfig_.txBuf = txBuf;
        bufferConfig_.rxBuf = rxBuf;
        bufferConfig_.txLength = size;
        bufferConfig_.rxLength = size;

        status_.dmaMode = false;
        status_.transferLength = size;
        status_.txOngoing = true;
        status_.rxOngoing = true;

	    SSI_INTConfig(SPI_DEV_TABLE[spiInterface_].SPIx, (BIT_IMR_RXFIM | BIT_IMR_RXOIM | BIT_IMR_RXUIM), ENABLE);
        SSI_INTConfig(SPI_DEV_TABLE[spiInterface_].SPIx, (BIT_IMR_TXOIM | BIT_IMR_TXEIM), ENABLE);

        // Wait for SPI transfer finished
        while (isBusy()) {
            ;
        }

        return SYSTEM_ERROR_NONE;
    }

    int transferDma(const uint8_t* txBuf, uint8_t* rxBuf, size_t size, hal_spi_dma_user_callback callback) {
        CHECK_TRUE(isEnabled(), SYSTEM_ERROR_INVALID_STATE);
        CHECK_TRUE(config_.spiMode == SPI_MODE_MASTER, SYSTEM_ERROR_INVALID_STATE);

        // Wait for last SPI transfer finished
        while (isBusy()) {
            ;
        }

        bufferConfig_.rxBuf = rxBuf;
        bufferConfig_.rxLength = size;
        bufferConfig_.txBuf = txBuf;
        bufferConfig_.txLength = size;

        callbackConfig_.dmaUserCb = callback;

        status_.dmaMode = true;
        status_.transferLength = size;
        status_.txOngoing = true;
        status_.rxOngoing = true;

        NVIC_SetPriority(GDMA_GetIrqNum(0, rxGdma_.GDMA_ChNum), CFG_GDMA_RX_PRIORITY);
        NVIC_SetPriority(GDMA_GetIrqNum(0, txGdma_.GDMA_ChNum), CFG_GDMA_TX_PRIORITY);
        SSI_SetDmaEnable(SPI_DEV_TABLE[spiInterface_].SPIx, ENABLE, BIT_SHIFT_DMACR_RDMAE);
        SSI_SetDmaEnable(SPI_DEV_TABLE[spiInterface_].SPIx, ENABLE, BIT_SHIFT_DMACR_TDMAE);
        SSI_RXGDMA_Init(spiInterface_, &rxGdma_, this, (IRQ_FUN)dmaRxHandler, bufferConfig_.rxBuf, bufferConfig_.rxLength);
        SSI_TXGDMA_Init(spiInterface_, &txGdma_, this, (IRQ_FUN)dmaTxHandler, (uint8_t*)bufferConfig_.txBuf, bufferConfig_.txLength);

        return SYSTEM_ERROR_NONE;
    }

    int transferDmaCancel() {
        CHECK_TRUE(isEnabled(), SYSTEM_ERROR_INVALID_STATE);
        CHECK_TRUE(isBusy() && status_.dmaMode, SYSTEM_ERROR_INVALID_STATE);

        /* Disable interrupt */
        SSI_INTConfig(SPI_DEV_TABLE[spiInterface_].SPIx, (BIT_IMR_RXFIM | BIT_IMR_RXOIM | BIT_IMR_RXUIM), DISABLE);

        /* Set SSI DMA Disable */
        SSI_SetDmaEnable(SPI_DEV_TABLE[spiInterface_].SPIx, DISABLE, BIT_SHIFT_DMACR_RDMAE);

        /* Clear Pending ISR */
        GDMA_ClearINT(rxGdma_.GDMA_Index, rxGdma_.GDMA_ChNum);
        GDMA_ChCleanAutoReload(rxGdma_.GDMA_Index, rxGdma_.GDMA_ChNum, CLEAN_RELOAD_SRC_DST);
        GDMA_Cmd(rxGdma_.GDMA_Index, rxGdma_.GDMA_ChNum, DISABLE);
        GDMA_ChnlFree(rxGdma_.GDMA_Index, rxGdma_.GDMA_ChNum);

        u32 rxFifoLevel;
        while (SSI_Readable(SPI_DEV_TABLE[spiInterface_].SPIx)) {
            rxFifoLevel = SSI_GetRxCount(SPI_DEV_TABLE[spiInterface_].SPIx);
            for (u32 i = 0; i < rxFifoLevel; i++) {
                SSI_ReadData(SPI_DEV_TABLE[spiInterface_].SPIx);
            }
        }

        return SYSTEM_ERROR_NONE;
    }

    SpiConfig config() const {
        return config_;
    }

    int setConfig(SpiConfig& config) {
        CHECK_TRUE(validateConfig(spiInterface_, config), SYSTEM_ERROR_INVALID_ARGUMENT);
        memcpy(&config_, &config, sizeof(SpiConfig));
        return SYSTEM_ERROR_NONE;
    }

    SpiStatus status() const {
        return status_;
    }

    hal_pin_t csPin() const {
        return csPin_;
    }

    bool isEnabled() const {
        return status_.state == HAL_SPI_STATE_ENABLED;
    }

    bool isBusy() const {
        return status_.txOngoing || status_.rxOngoing;
    }

    bool isSuspended() const {
        return status_.state == HAL_SPI_STATE_SUSPENDED;
    }

    int registerSelectUserCb(hal_spi_select_user_callback callback) {
        callbackConfig_.selectUserCb = callback;
        return SYSTEM_ERROR_NONE;
    }

    void interruptHandlerImpl() {
        u32 interruptStatus = SSI_GetIsr(SPI_DEV_TABLE[spiInterface_].SPIx);

        SSI_SetIsrClean(SPI_DEV_TABLE[spiInterface_].SPIx, interruptStatus);

        if (interruptStatus & (BIT_ISR_TXOIS | BIT_ISR_RXUIS | BIT_ISR_RXOIS | BIT_ISR_MSTIS)) {
            LOG_DEBUG(WARN, "SPI error 0x%08x", interruptStatus);
        }

        if ((interruptStatus & BIT_ISR_RXFIS)) {
            LOG(INFO, "[int] spi: BIT_ISR_RXFIS!");
            u32 bytesReceived = 0;
            if (bufferConfig_.rxBuf) {
                // TODO: overflow protection
                bytesReceived = SSI_ReceiveData(SPI_DEV_TABLE[spiInterface_].SPIx, bufferConfig_.rxBuf, bufferConfig_.rxLength);
                bufferConfig_.rxBuf += bytesReceived;
                bufferConfig_.rxLength -= bytesReceived;
            }

            if (bufferConfig_.rxLength == 0) {
                SSI_INTConfig(SPI_DEV_TABLE[spiInterface_].SPIx, (BIT_IMR_RXFIM | BIT_IMR_RXOIM | BIT_IMR_RXUIM), DISABLE);
                status_.rxOngoing = false;
            }
        }

        if (interruptStatus & BIT_ISR_TXEIS) {
            LOG(INFO, "[int] spi: BIT_ISR_TXEIS!");
            u32 bytesSent = 0;
            volatile u32 bus_busy;
            u32 i;

            /* all data complete */
            if (bufferConfig_.txLength == 0) {
                SSI_INTConfig(SPI_DEV_TABLE[spiInterface_].SPIx, (BIT_IMR_TXOIM | BIT_IMR_TXEIM), DISABLE);
                // FIXME: do we need this?
                for (i = 0; i < 1000000; i++) {
                    bus_busy = SSI_Busy(SPI_DEV_TABLE[spiInterface_].SPIx);
                    if (!bus_busy) {
                        break; // break the for loop
                    }
                }
                return;
            }

            bytesSent = SSI_SendData(SPI_DEV_TABLE[spiInterface_].SPIx, (void*)bufferConfig_.txBuf, bufferConfig_.txLength,
                                    (config_.spiMode == SPI_MODE_MASTER) ? SSI_MASTER : SSI_SLAVE);
            LOG(INFO, "int: bytesSent: %d, data[0]: 0x%x", bytesSent, bufferConfig_.txBuf[0]);
            if (bufferConfig_.txBuf) {
                bufferConfig_.txBuf += bytesSent;
                bufferConfig_.txLength -= bytesSent;
            }

            /* all data write into fifo */
            if (bufferConfig_.txLength == 0) {
                SSI_INTConfig(SPI_DEV_TABLE[spiInterface_].SPIx, (BIT_IMR_TXOIM), DISABLE);
                status_.txOngoing = false;
            }
        }
    }

    void dmaTxHandlerImpl() {
        // Clear Pending ISR
        GDMA_ClearINT(txGdma_.GDMA_Index, txGdma_.GDMA_ChNum);
        GDMA_Cmd(txGdma_.GDMA_Index, txGdma_.GDMA_ChNum, DISABLE);
        SSI_SetDmaEnable(SPI_DEV_TABLE[spiInterface_].SPIx, DISABLE, BIT_SHIFT_DMACR_TDMAE);
        GDMA_ChnlFree(txGdma_.GDMA_Index, txGdma_.GDMA_ChNum);
        status_.txOngoing = false;
        LOG(INFO, "[int] spi dma tx!");
    }

    void dmaRxHandlerImpl() {
        size_t Length = bufferConfig_.rxLength;
        uint8_t* pRxData = bufferConfig_.rxBuf;

        /* Clear Pending ISR */
        GDMA_ClearINT(rxGdma_.GDMA_Index, rxGdma_.GDMA_ChNum);
        GDMA_Cmd(rxGdma_.GDMA_Index, rxGdma_.GDMA_ChNum, DISABLE);

        DCache_Invalidate((u32) pRxData, Length);

        /* Set SSI DMA Disable */
        SSI_SetDmaEnable(SPI_DEV_TABLE[spiInterface_].SPIx, DISABLE, BIT_SHIFT_DMACR_RDMAE);
        GDMA_ChnlFree(rxGdma_.GDMA_Index, rxGdma_.GDMA_ChNum);

        status_.rxOngoing = false;
        LOG(INFO, "[int] spi dma rx!");
        if (callbackConfig_.dmaUserCb) {
            (*callbackConfig_.dmaUserCb)();
        }
    }

    void onSelectedHandlerImpl() {
        status_.csPinSelected = !hal_gpio_read(csPin_);
        if (callbackConfig_.selectUserCb) {
            (*callbackConfig_.selectUserCb)(status_.csPinSelected);
        }
    }

    static bool validateConfig(hal_spi_interface_t spi, const SpiConfig& config) {
        CHECK_TRUE(config.spiMode == SPI_MODE_MASTER ||
                   config.spiMode == SPI_MODE_SLAVE, false);
        CHECK_TRUE(config.dataMode == SPI_MODE0 ||
                   config.dataMode == SPI_MODE1 ||
                   config.dataMode == SPI_MODE2 ||
                   config.dataMode == SPI_MODE3, false);
        CHECK_TRUE(config.bitOrder == MSBFIRST ||
                   config.bitOrder == LSBFIRST, false);
        // SPI0 can work as master and slave while SPI1 can only work as slave.
        CHECK_FALSE(spi == HAL_SPI_INTERFACE2 && config.spiMode == SPI_MODE_SLAVE, false);
        return true;
    }

    static uint32_t interruptHandler(void* context) {
        Spi* spiInstance = (Spi*)context;
        spiInstance->interruptHandlerImpl();
        return SYSTEM_ERROR_NONE;
    }

    static void dmaTxHandler(void* context) {
        Spi* spiInstance = (Spi*)context;
        spiInstance->dmaTxHandlerImpl();
    }

    static void dmaRxHandler(void* context) {
        Spi* spiInstance = (Spi*)context;
        spiInstance->dmaRxHandlerImpl();
    }

    static void onSelectedHandler(void* context) {
        Spi* spiInstance = (Spi*)context;
        spiInstance->onSelectedHandlerImpl();
    }

private:
    hal_spi_interface_t     spiInterface_;
    int                     prio_;
    hal_pin_t               csPin_;
    hal_pin_t               sclkPin_;
    hal_pin_t               mosiPin_;
    hal_pin_t               misoPin_;

    SpiConfig               config_;
    SpiBufferConfig         bufferConfig_;
    SpiCallbackConfig       callbackConfig_;

    volatile uint8_t        ssPinState_;

    GDMA_InitTypeDef        rxGdma_;
    GDMA_InitTypeDef        txGdma_;
    os_mutex_recursive_t    mutex_;
    SpiStatus               status_;
};

Spi* getInstance(hal_spi_interface_t spi) {
    static Spi spiMap[] = {
        {HAL_SPI_INTERFACE1, CFG_SPI_PRIORITY, SS, SCK, MOSI, MISO},
        {HAL_SPI_INTERFACE2, CFG_SPI_PRIORITY, SS1, SCK1, MOSI1, MISO1}
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

__attribute__(( optimize("-O0") )) void hal_spi_begin(hal_spi_interface_t spi, uint16_t pin) {
    // Default to Master mode
    hal_spi_begin_ext(spi, SPI_MODE_MASTER, pin, nullptr);
}

void hal_spi_begin_ext(hal_spi_interface_t spi, hal_spi_mode_t mode, uint16_t pin, void* reserved) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return;
    }
    auto spiInstance = getInstance(spi);
    SpiConfig config = {};
    config.reset();
    config.spiMode = mode;
    spiInstance->begin(pin, config);
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
}

void hal_spi_set_data_mode(hal_spi_interface_t spi, uint8_t mode) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return;
    }
    auto spiInstance = getInstance(spi);
    auto config = spiInstance->config();
    config.bitOrder = mode;
    spiInstance->setConfig(config);
}

void hal_spi_set_clock_divider(hal_spi_interface_t spi, uint8_t rate) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return;
    }
    auto spiInstance = getInstance(spi);
    auto config = spiInstance->config();
    config.clockDiv = rate;
    spiInstance->setConfig(config);
}

uint16_t hal_spi_transfer(hal_spi_interface_t spi, uint16_t data) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return 0;
    }
    auto spiInstance = getInstance(spi);
    uint8_t rxData = 0;
    spiInstance->transfer((uint8_t*)&data, &rxData, 1);
    return rxData;
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

    info->system_clock = (spi == HAL_SPI_INTERFACE1) ? 100000000 : 50000000;

    auto spiInstance = getInstance(spi);
    auto config = spiInstance->config();
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
    int bytesToBeSent = spiInstance->isBusy() ? 0 : status.transferLength;

    if (st) {
        st->ss_state = status.csPinSelected;
        st->transfer_ongoing = spiInstance->isBusy();
        st->configured_transfer_length = status.transferLength;
        st->transfer_length = bytesToBeSent;
    }

    return bytesToBeSent;
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
    return spiInstance->begin(config);
}

int hal_spi_sleep(hal_spi_interface_t spi, bool sleep, void* reserved) {
    auto spiInstance = CHECK_TRUE_RETURN(getInstance(spi), SYSTEM_ERROR_NOT_FOUND);
    if (sleep) {
        CHECK_TRUE(hal_spi_is_enabled(spi), SYSTEM_ERROR_INVALID_STATE);
        while (spiInstance->isBusy());
        spiInstance->end();
    } else  {
        CHECK_TRUE(spiInstance->isSuspended(), SYSTEM_ERROR_INVALID_STATE);
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

    // IpClk for SPI1 is 50MHz and IpClk for SPI0 is 100MHz
    u32 IpClk;
    if (spi == HAL_SPI_INTERFACE1) {
        IpClk = 100000000;
    } else {
        IpClk = 50000000;
    }

    // "clock" should be less or equal to half of the SPI IpClk.
    CHECK_TRUE(clock <= IpClk, SYSTEM_ERROR_INVALID_ARGUMENT);

    // Integer division results in clean values
    switch (IpClk / clock) {
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
