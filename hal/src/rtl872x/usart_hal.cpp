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
#include "usart_hal.h"
#include "ringbuffer.h"
#include "pinmap_hal.h"
#include "gpio_hal.h"
#include <algorithm>
#include <memory>
#include "hal_irq_flag.h"
#include "delay_hal.h"
#include "interrupts_hal.h"
#include "usart_hal_private.h"
#include "timer_hal.h"
#include "service_debug.h"
#include "atomic_section.h"
#include "flash_common.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "rtl8721d.h"
#ifdef __cplusplus
}
#endif

namespace {

constexpr uint32_t BIT_CFGX_LO_FIFO_EMPTY = (1 << 9);
constexpr uint32_t BIT_CFGX_LO_CH_SUSP = (1 << 8);

void dcacheInvalidateAligned(uintptr_t ptr, size_t size) {
    uintptr_t alignedPtr = ptr & ~(portBYTE_ALIGNMENT_MASK);
    uintptr_t end = ptr + size;
    size_t alignedSize = CEIL_DIV(end - alignedPtr, portBYTE_ALIGNMENT) * portBYTE_ALIGNMENT;
    DCache_Invalidate(alignedPtr, alignedSize);
}

// Do not initiate DMA RX transfers is acquirable space is below this threshold
// This is the same as on Gen 3 to bring similar behavior
constexpr size_t RX_THRESHOLD = 4;

} // anonymous


class Usart {
public:
    struct Config {
        unsigned int baudRate;
        unsigned int config;
    };

    class AtomicBlock {
    public:
        AtomicBlock(Usart* instance)
                : uart_(instance) {
            NVIC_DisableIRQ(uart_->uartTable_[uart_->index_].IrqNum);
        }
        ~AtomicBlock() {
            NVIC_EnableIRQ(uart_->uartTable_[uart_->index_].IrqNum);
        }
    private:
        Usart* uart_;
    };

    class RxLock {
    public:
        RxLock(Usart* instance)
                : uart_(instance) {
            uart_->rxLock(true);
        }
        ~RxLock() {
            uart_->rxLock(false);
        }
    private:
        Usart* uart_;
    };

    class TxLock {
    public:
        TxLock(Usart* instance)
                : uart_(instance) {
            uart_->txLock(true);
        }
        ~TxLock() {
            uart_->txLock(false);
        }
    private:
        Usart* uart_;
    };

    bool isEnabled() const {
        return state_ == HAL_USART_STATE_ENABLED;
    }

    bool isConfigured() const {
        return configured_;
    }

    int init(const hal_usart_buffer_config_t& conf) {
        if (isEnabled()) {
            flush();
            CHECK(end());
        }
        if (isConfigured()) {
            CHECK(deInit());
        }
        CHECK_TRUE(conf.rx_buffer, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(conf.rx_buffer_size, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(conf.tx_buffer, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(conf.tx_buffer_size, SYSTEM_ERROR_INVALID_ARGUMENT);
        // Caching routines require 32-byte aligned buffers
        // Normally heap allocations (and normally buffers come out of heap)
        // should be aligned to portBYTE_ALIGNMENT (which is defined to 32 on RTL872x port)
        // Adjust here if required
        void* rxBuf = conf.rx_buffer;
        size_t rxBufSize = conf.rx_buffer_size;
        void* txBuf = conf.tx_buffer;
        size_t txBufSize = conf.tx_buffer_size;
        if (((uintptr_t)rxBuf & portBYTE_ALIGNMENT_MASK) != 0) {
            rxBuf = std::align(portBYTE_ALIGNMENT, conf.rx_buffer_size, rxBuf, rxBufSize);
            rxBufSize = (rxBufSize / portBYTE_ALIGNMENT) * portBYTE_ALIGNMENT;
        }
        if (((uintptr_t)txBuf & portBYTE_ALIGNMENT_MASK) != 0) {
            txBuf = std::align(portBYTE_ALIGNMENT, conf.tx_buffer_size, txBuf, txBufSize);
            txBufSize = (txBufSize / portBYTE_ALIGNMENT) * portBYTE_ALIGNMENT;
        }
        CHECK_TRUE(rxBuf, SYSTEM_ERROR_NOT_ENOUGH_DATA);
        CHECK_TRUE(rxBufSize >= portBYTE_ALIGNMENT, SYSTEM_ERROR_NOT_ENOUGH_DATA);
        CHECK_TRUE(txBuf, SYSTEM_ERROR_NOT_ENOUGH_DATA);
        CHECK_TRUE(txBufSize >= portBYTE_ALIGNMENT, SYSTEM_ERROR_NOT_ENOUGH_DATA);
        rxBuffer_.init((uint8_t*)rxBuf, rxBufSize);
        txBuffer_.init((uint8_t*)txBuf, txBufSize);
        DCache_CleanInvalidate((uintptr_t)rxBuf, rxBufSize);
        DCache_CleanInvalidate((uintptr_t)txBuf, txBufSize);
        configured_ = true;
        return SYSTEM_ERROR_NONE;
    }

    int deInit() {
        configured_ = false;
        return SYSTEM_ERROR_NONE;
    }

    int begin(const Config& conf) {
        CHECK_TRUE(isConfigured(), SYSTEM_ERROR_INVALID_STATE);
        CHECK_TRUE(validateConfig(conf.config), SYSTEM_ERROR_INVALID_ARGUMENT);
        if (isEnabled()) {
            flush();
            end();
        }
        auto uartInstance = uartTable_[index_].UARTx;
        // Enable peripheral clock
        if (uartInstance == UART0_DEV) {
            RCC_PeriphClockCmd(APBPeriph_UART0, APBPeriph_UART0_CLOCK, ENABLE);
        }
        // Configure TX/RX pins
        if (uartInstance == UART2_DEV) {
            Pinmux_Config(hal_pin_to_rtl_pin(txPin_), PINMUX_FUNCTION_LOGUART);
            Pinmux_Config(hal_pin_to_rtl_pin(rxPin_), PINMUX_FUNCTION_LOGUART);
        } else {
            Pinmux_Config(hal_pin_to_rtl_pin(txPin_), PINMUX_FUNCTION_UART);
            Pinmux_Config(hal_pin_to_rtl_pin(rxPin_), PINMUX_FUNCTION_UART);
        }
        PAD_PullCtrl(hal_pin_to_rtl_pin(txPin_), GPIO_PuPd_UP);
        PAD_PullCtrl(hal_pin_to_rtl_pin(rxPin_), GPIO_PuPd_UP);

        UART_InitTypeDef uartInitStruct = {};
        UART_StructInit(&uartInitStruct);
        uartInitStruct.RxFifoTrigLevel = UART_RX_FIFOTRIG_LEVEL_1BYTES;
        UART_Init(uartInstance, &uartInitStruct);

        RCC_PeriphClockSource_UART(uartInstance, UART_RX_CLK_XTAL_40M);
        UART_SetBaud(uartInstance, conf.baudRate);
        UART_RxCmd(uartInstance, ENABLE);

        UART_RxCmd(uartInstance, DISABLE);
        // Data bits, stop bits and parity
        if ((conf.config & SERIAL_DATA_BITS) == SERIAL_DATA_BITS_8) {
            uartInitStruct.WordLen = RUART_WLS_8BITS;
        } else {
            uartInitStruct.WordLen = RUART_WLS_7BITS;
        }
        if ((conf.config & SERIAL_STOP_BITS) == SERIAL_STOP_BITS_2) {
            uartInitStruct.StopBit = RUART_STOP_BIT_2;
        } else {
            uartInitStruct.StopBit = RUART_STOP_BIT_1;
        }
        switch (conf.config & SERIAL_PARITY) {
            case SERIAL_PARITY_ODD: {
                uartInitStruct.Parity = RUART_PARITY_ENABLE;
                uartInitStruct.ParityType = RUART_ODD_PARITY;
                break;
            }
            case SERIAL_PARITY_EVEN: {
                uartInitStruct.Parity = RUART_PARITY_ENABLE;
                uartInitStruct.ParityType = RUART_EVEN_PARITY;
                break;
            }
            default: { // ParityNone
                uartInitStruct.Parity = RUART_PARITY_DISABLE;
                break;
            }
        }
        uartInstance->LCR = ((uartInitStruct.WordLen) | (uartInitStruct.StopBit << 2) |
                             (uartInitStruct.Parity << 3) | (uartInitStruct.ParityType << 4) | (uartInitStruct.StickParity << 5));
        UART_RxCmd(uartInstance, ENABLE);

        if ((ctsPin_ != PIN_INVALID || rtsPin_ != PIN_INVALID) &&
            ((conf.config & SERIAL_FLOW_CONTROL) != SERIAL_FLOW_CONTROL_NONE)) {
            // Init CTS in low
            PAD_PullCtrl(hal_pin_to_rtl_pin(ctsPin_), GPIO_PuPd_DOWN);
            Pinmux_Config(hal_pin_to_rtl_pin(ctsPin_), PINMUX_FUNCTION_UART_RTSCTS);
            Pinmux_Config(hal_pin_to_rtl_pin(rtsPin_), PINMUX_FUNCTION_UART_RTSCTS);
            uartInstance->MCR |= BIT(5);
            uartInstance->MCR |= BIT(1);    // RTS low
        } else {
            uartInstance->MCR &= ~ BIT(5);
        }

        // Configuring DMA
        if (!initDmaChannels()) {
            end();
            return SYSTEM_ERROR_INTERNAL;
        }

        startReceiver();

        config_ = conf;
        state_ = HAL_USART_STATE_ENABLED;
        return SYSTEM_ERROR_NONE;
    }

    int end() {
        CHECK_TRUE(state_ != HAL_USART_STATE_DISABLED, SYSTEM_ERROR_INVALID_STATE);
        auto uartInstance = uartTable_[index_].UARTx;
        deinitDmaChannels();
        UART_ClearTxFifo(uartInstance);
        UART_ClearRxFifo(uartInstance);
        UART_DeInit(uartInstance);
        if (uartInstance == UART0_DEV) {
            RCC_PeriphClockCmd(APBPeriph_UART0, APBPeriph_UART0_CLOCK, DISABLE);
        }
        // Do not change the pull ability to not mess up the peer device.
        Pinmux_Config(hal_pin_to_rtl_pin(txPin_), PINMUX_FUNCTION_GPIO);
        Pinmux_Config(hal_pin_to_rtl_pin(rxPin_), PINMUX_FUNCTION_GPIO);
        if (ctsPin_ != PIN_INVALID) {
            Pinmux_Config(hal_pin_to_rtl_pin(ctsPin_), PINMUX_FUNCTION_GPIO);
        }
        if (rtsPin_ != PIN_INVALID) {
            Pinmux_Config(hal_pin_to_rtl_pin(rtsPin_), PINMUX_FUNCTION_GPIO);
        }
        config_ = {};
        rxBuffer_.reset();
        txBuffer_.reset();
        curTxCount_ = 0;
        transmitting_ = false;
        receiving_ = false;
        state_ = HAL_USART_STATE_DISABLED;
        return SYSTEM_ERROR_NONE;
    }

    ssize_t space() {
        CHECK_TRUE(isEnabled(), SYSTEM_ERROR_INVALID_STATE);
        TxLock lk(this);
        return txBuffer_.space();
    }

    ssize_t write(const uint8_t* buffer, size_t size) {
        CHECK_TRUE(buffer, SYSTEM_ERROR_INVALID_ARGUMENT);
        const ssize_t canWrite = CHECK(space());
        const size_t writeSize = std::min((size_t)canWrite, size);
        CHECK_TRUE(writeSize > 0, SYSTEM_ERROR_NO_MEMORY);
        
        ssize_t r;
        {
            TxLock lk(this);
            r = CHECK(txBuffer_.put(buffer, writeSize));
        }

        startTransmission();
        return r;
    }

    ssize_t flush() {
        startTransmission();
        while (true) {
            {
                TxLock lk(this);
                if (!isEnabled() || txBuffer_.empty()) {
                    break;
                }
            }
            CHECK(pollStatus());
        }
        return 0;
    }

    size_t dataInFlight(bool commit = false, bool cancel = false) {
        // Must be called under RX lock!
        if (receiving_) {
            auto uartInstance = uartTable_[index_].UARTx;
            size_t transferredToDmaFromUart = uartInstance->RX_BYTE_CNT & RUART_RX_READ_BYTE_CNTER;
            size_t dmaAvailableInBuffer = GDMA_GetDstAddr(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum) - rxDmaInitStruct_.GDMA_DstAddr;
            size_t blockSize = rxDmaInitStruct_.GDMA_BlockSize;
            size_t alreadyCommitted = blockSize - rxBuffer_.acquirePending();
            ssize_t toConsume = std::max<ssize_t>(dmaAvailableInBuffer, transferredToDmaFromUart) - alreadyCommitted;
            SPARK_ASSERT(toConsume >= 0);
            if (commit && toConsume > 0) {
                if (transferredToDmaFromUart > dmaAvailableInBuffer) {
                    uintptr_t expectedDstAddr = rxDmaInitStruct_.GDMA_DstAddr + transferredToDmaFromUart;
                    if ((GDMA_BASE->CH[rxDmaInitStruct_.GDMA_ChNum].CFG_LOW & BIT_CFGX_LO_FIFO_EMPTY) == 0) {
                        if (GDMA_GetDstAddr(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum) < expectedDstAddr) {
                            // Suspending DMA channel forces flushing of data into destination from GDMA FIFO
                            GDMA_BASE->CH[rxDmaInitStruct_.GDMA_ChNum].CFG_LOW |= BIT_CFGX_LO_CH_SUSP;
                            __DSB();
                            __ISB();
                            while (GDMA_GetDstAddr(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum) < expectedDstAddr) {
                                // XXX: spin around, this should be pretty fast
                            }
                            GDMA_BASE->CH[rxDmaInitStruct_.GDMA_ChNum].CFG_LOW &= ~(BIT_CFGX_LO_CH_SUSP);
                            __DSB();
                            __ISB();
                        }
                    }
                }
                rxBuffer_.acquireCommit(toConsume);
                if (cancel) {
                    // Release the rest of the buffer if any
                    rxBuffer_.acquireCommit(0, rxBuffer_.acquirePending());
                }
                dcacheInvalidateAligned(rxDmaInitStruct_.GDMA_DstAddr + alreadyCommitted, toConsume);
            }
            return toConsume;
        }
        return 0;
    }

    ssize_t data() {
        CHECK_TRUE(isEnabled(), SYSTEM_ERROR_INVALID_STATE);
        ssize_t len = 0;
        RxLock lk(this);
        len = CHECK(rxBuffer_.data()) + dataInFlight();
        return len;
    }

    ssize_t read(uint8_t* buffer, size_t size) {
        CHECK_TRUE(isEnabled(), SYSTEM_ERROR_INVALID_STATE);
        const ssize_t maxRead = CHECK(data());
        const ssize_t readSize = std::min((size_t)maxRead, size);
        CHECK_TRUE(readSize > 0, SYSTEM_ERROR_NO_MEMORY);
        RxLock lk(this);
        if (readSize > rxBuffer_.data()) {
            dataInFlight(true /* commit */);
        }
        SPARK_ASSERT(rxBuffer_.data() >= readSize);
        ssize_t r = CHECK(rxBuffer_.get(buffer, readSize));
        if (!receiving_) {
            startReceiver();
        }
        return r;
    }

    ssize_t peek(uint8_t* buffer, size_t size) {
        CHECK_TRUE(isEnabled(), SYSTEM_ERROR_INVALID_STATE);
        const ssize_t maxRead = CHECK(data());
        const ssize_t peekSize = std::min((size_t)maxRead, size);
        CHECK_TRUE(peekSize > 0, SYSTEM_ERROR_NO_MEMORY);
        RxLock lk(this);
        if (peekSize > rxBuffer_.data()) {
            dataInFlight(true /* commit */);
        }
        SPARK_ASSERT(rxBuffer_.data() >= peekSize);
        return rxBuffer_.peek(buffer, peekSize);
    }

    int pollStatus() {
        TxLock lk(this);
        uartTxDmaCompleteHandler(this);
        return SYSTEM_ERROR_NONE;
    }

    static Usart* getInstance(hal_usart_interface_t serial) {
        static Usart Usarts[] = {
            {2, TX,  RX,  PIN_INVALID, PIN_INVALID}, // LOG UART
            {0, TX1, RX1, CTS1,        RTS1}, // UART0
            {3, TX2, RX2, CTS2,        RTS2} // LP_UART
        };
        CHECK_TRUE(serial < sizeof(Usarts) / sizeof(Usarts[0]), nullptr);
        return &Usarts[serial];
    }

private:
    Usart(uint8_t index, hal_pin_t txPin, hal_pin_t rxPin, hal_pin_t ctsPin, hal_pin_t rtsPin)
            : txPin_(txPin),
              rxPin_(rxPin),
              ctsPin_(ctsPin),
              rtsPin_(rtsPin),
              txDmaInitStruct_{},
              rxDmaInitStruct_{},
              config_(),
              curTxCount_(0),
              state_(HAL_USART_STATE_DISABLED),
              transmitting_(false),
              receiving_(false),
              configured_(false),
              index_(index) {
        // LOG UART is enabled on boot
        if (index_ == 2) {
            state_ = HAL_USART_STATE_ENABLED;
        }
    }
    ~Usart() = default;

    bool validateConfig(unsigned int config) {
        CHECK_TRUE((config & SERIAL_DATA_BITS) == SERIAL_DATA_BITS_7 ||
                   (config & SERIAL_DATA_BITS) == SERIAL_DATA_BITS_8, false);
        CHECK_TRUE((config & SERIAL_STOP_BITS) == SERIAL_STOP_BITS_1 ||
                   (config & SERIAL_STOP_BITS) == SERIAL_STOP_BITS_2, false);
        CHECK_TRUE((config & SERIAL_PARITY) == SERIAL_PARITY_NO ||
                   (config & SERIAL_PARITY) == SERIAL_PARITY_EVEN ||
                   (config & SERIAL_PARITY) == SERIAL_PARITY_ODD, false);
        CHECK_TRUE((config & SERIAL_HALF_DUPLEX) == 0, false);
        CHECK_TRUE((config & LIN_MODE) == 0, false);
        return true;
    }

    bool initDmaChannels() {
        auto uartInstance = uartTable_[index_].UARTx;
        UART_TXDMAConfig(uartInstance, 8);
        UART_TXDMACmd(uartInstance, ENABLE);

        _memset(&txDmaInitStruct_, 0, sizeof(GDMA_InitTypeDef));
        _memset(&rxDmaInitStruct_, 0, sizeof(GDMA_InitTypeDef));
        txDmaInitStruct_.GDMA_ChNum = 0xFF;
        rxDmaInitStruct_.GDMA_ChNum = 0xFF;
        uint8_t txChannel = GDMA_ChnlAlloc(0, (IRQ_FUN)uartTxDmaCompleteHandler, (uint32_t)this, 12);//ACUT is 0x10, BCUT is 12
        if (txChannel == 0xFF) {
            return false;
        }
        uint8_t rxChannel = GDMA_ChnlAlloc(0, (IRQ_FUN)uartRxDmaCompleteHandler, (uint32_t)this, 12);
        if (rxChannel == 0xFF) {
            GDMA_ChnlFree(0, txChannel);
            txChannel = 0xFF;
            return false;
        }
        txDmaInitStruct_.MuliBlockCunt = 0;
        txDmaInitStruct_.MaxMuliBlock = 1;
        txDmaInitStruct_.GDMA_DIR = TTFCMemToPeri;
        txDmaInitStruct_.GDMA_DstHandshakeInterface = uartTable_[index_].Tx_HandshakeInterface;
        txDmaInitStruct_.GDMA_DstAddr = (uint32_t)&uartInstance->RB_THR;
        txDmaInitStruct_.GDMA_Index = 0;
        txDmaInitStruct_.GDMA_ChNum = txChannel;
        txDmaInitStruct_.GDMA_IsrType = (BlockType|TransferType|ErrType);
        txDmaInitStruct_.GDMA_DstMsize = MsizeFour;
        txDmaInitStruct_.GDMA_DstDataWidth = TrWidthOneByte;
        txDmaInitStruct_.GDMA_DstInc = NoChange;
        txDmaInitStruct_.GDMA_SrcInc = IncType;

        rxDmaInitStruct_.MuliBlockCunt = 0;
        rxDmaInitStruct_.MaxMuliBlock = 1;
        rxDmaInitStruct_.GDMA_DIR = TTFCPeriToMem;
        rxDmaInitStruct_.GDMA_ReloadSrc = 0;
        rxDmaInitStruct_.GDMA_SrcHandshakeInterface = uartTable_[index_].Rx_HandshakeInterface;
        rxDmaInitStruct_.GDMA_SrcAddr = (uint32_t)&uartInstance->RB_THR;
        rxDmaInitStruct_.GDMA_Index = 0;
        rxDmaInitStruct_.GDMA_ChNum = rxChannel;
        rxDmaInitStruct_.GDMA_IsrType = (BlockType|TransferType|ErrType);
        rxDmaInitStruct_.GDMA_SrcMsize = MsizeOne;
        rxDmaInitStruct_.GDMA_SrcDataWidth = TrWidthOneByte;
        rxDmaInitStruct_.GDMA_DstInc = IncType;
        rxDmaInitStruct_.GDMA_SrcInc = NoChange;
        rxDmaInitStruct_.GDMA_DstMsize = MsizeOne;
        rxDmaInitStruct_.GDMA_DstDataWidth = TrWidthOneByte;

        NVIC_SetPriority(GDMA_GetIrqNum(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum), 12);
        NVIC_SetPriority(GDMA_GetIrqNum(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum), 12);
        return true;
    }

    void deinitDmaChannels() {
        auto uartInstance = uartTable_[index_].UARTx;
        GDMA_Cmd(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, DISABLE);
        GDMA_Cmd(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, DISABLE);
        // Clean Auto Reload Bit
        GDMA_ChCleanAutoReload(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, CLEAN_RELOAD_DST);
        GDMA_ChCleanAutoReload(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, CLEAN_RELOAD_SRC);
        // Clear Pending ISR
        GDMA_ClearINT(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum);
        GDMA_ClearINT(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum);
        if (txDmaInitStruct_.GDMA_ChNum != 0xFF) {
            GDMA_ChnlFree(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum);
        }
        if (rxDmaInitStruct_.GDMA_ChNum != 0xFF) {
            GDMA_ChnlFree(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum);
        }
        UART_TXDMACmd(uartInstance, DISABLE);
        UART_RXDMACmd(uartInstance, DISABLE);
    }

    void startTransmission() {
        size_t consumable;
        auto uartInstance = uartTable_[index_].UARTx;
        if (!transmitting_ && (consumable = txBuffer_.consumable())) {
            transmitting_ = true;
            if (txDmaInitStruct_.GDMA_ChNum == 0xFF) {
                transmitting_ = false;
                return;
            }
            consumable = std::min(MAX_DMA_BLOCK_SIZE, consumable);
            auto ptr = txBuffer_.consume(consumable);

            DCache_CleanInvalidate((uint32_t)ptr, consumable);

            if (((consumable & 0x03) == 0) && (((uint32_t)(ptr) & 0x03) == 0)) {
                // 4-bytes aligned, move 4 bytes each transfer
                txDmaInitStruct_.GDMA_SrcMsize   = MsizeOne;
                txDmaInitStruct_.GDMA_SrcDataWidth = TrWidthFourBytes;
                txDmaInitStruct_.GDMA_BlockSize = consumable >> 2;
            } else {
                // Move 1 byte each transfer
                txDmaInitStruct_.GDMA_SrcMsize   = MsizeFour;
                txDmaInitStruct_.GDMA_SrcDataWidth = TrWidthOneByte;
                txDmaInitStruct_.GDMA_BlockSize = consumable;
            }
            txDmaInitStruct_.GDMA_SrcAddr = (uint32_t)(ptr);
            curTxCount_ = consumable;
            GDMA_Init(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, &txDmaInitStruct_);
            GDMA_Cmd(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum, ENABLE);
        } else {
            // LOG UART doesn't support DMA transmission
            consumable = std::min(MAX_UART_FIFO_SIZE, consumable);
            auto ptr = txBuffer_.consume(consumable);
            for (size_t i = 0; i < consumable; i++, ptr++) {
                UART_CharPut(uartInstance, *ptr);
            }
            curTxCount_ = consumable;
            UART_INTConfig(uartInstance, RUART_IER_ETBEI, ENABLE);
        }
    }

    void startReceiver() {
        if (receiving_) {
            return;
        }
        receiving_ = true;
        auto uartInstance = uartTable_[index_].UARTx;
        if (rxDmaInitStruct_.GDMA_ChNum == 0xFF) {
            receiving_ = false;
            return;
        }
        // Updates current size
        rxBuffer_.acquireBegin();
        const size_t acquirable = rxBuffer_.acquirable();
        const size_t acquirableWrapped = rxBuffer_.acquirableWrapped();
        size_t rxSize = std::max(acquirable, acquirableWrapped);
        if (rxSize < RX_THRESHOLD) {
            receiving_ = false;
            return;
        }
        rxSize = std::min(MAX_DMA_BLOCK_SIZE, rxSize);
        // Disable Rx interrupt
        UART_INTConfig(uartInstance, (RUART_IER_ERBI | RUART_IER_ELSI | RUART_IER_ETOI), DISABLE);
        auto ptr = rxBuffer_.acquire(rxSize);

        UART_RXDMAConfig(uartInstance, 4);
        UART_RXDMACmd(uartInstance, ENABLE);

        // Configure GDMA as the flow controller
        uartInstance->MISCR &= ~(RUART_RXDMA_OWNER);
        rxDmaInitStruct_.GDMA_BlockSize = rxSize;
        rxDmaInitStruct_.GDMA_DstAddr = (uint32_t)(ptr);
        // Disable just in case, some of the settings are not applied if already enabled
        GDMA_Cmd(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, DISABLE);
        GDMA_Init(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, &rxDmaInitStruct_);
        GDMA_BurstEnable(rxDmaInitStruct_.GDMA_ChNum, DISABLE);
        // Clear UART RX FIFO read counter
        uartInstance->RX_BYTE_CNT |= RUART_RX_BYTE_CNTER_CLEAR;
        SPARK_ASSERT((uartInstance->RX_BYTE_CNT & RUART_RX_READ_BYTE_CNTER) == 0);
        GDMA_Cmd(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum, ENABLE);
    }

    static uint32_t uartTxDmaCompleteHandler(void* data) {
        auto uart = (Usart*)data;
        if (!uart->transmitting_) {
            return 0;
        }
        if (GDMA_BASE->STATUS_TFR != 0x00000000 || GDMA_BASE->STATUS_BLOCK != 0x00000000 || GDMA_BASE->STATUS_ERR != 0x00000000) {
            uart->transmitting_ = false;
            auto txDmaInitStruct = &uart->txDmaInitStruct_;
            // Clean Auto Reload Bit
            GDMA_ChCleanAutoReload(txDmaInitStruct->GDMA_Index, txDmaInitStruct->GDMA_ChNum, CLEAN_RELOAD_DST);
            // Clear Pending ISR
            GDMA_ClearINT(txDmaInitStruct->GDMA_Index, txDmaInitStruct->GDMA_ChNum);
            GDMA_Cmd(txDmaInitStruct->GDMA_Index, txDmaInitStruct->GDMA_ChNum, DISABLE);
            uart->txBuffer_.consumeCommit(uart->curTxCount_);
            uart->startTransmission();
        }
        return 0;
    }

    static uint32_t uartRxDmaCompleteHandler(void* data) {
        auto uart = (Usart*)data;
        if (!uart->receiving_) {
            return 0;
        }
        auto rxDmaInitStruct = &uart->rxDmaInitStruct_;
        // Clean Auto Reload Bit
        GDMA_ChCleanAutoReload(rxDmaInitStruct->GDMA_Index, rxDmaInitStruct->GDMA_ChNum, CLEAN_RELOAD_SRC);
        GDMA_Cmd(rxDmaInitStruct->GDMA_Index, rxDmaInitStruct->GDMA_ChNum, DISABLE);
        // Clear Pending ISR
        GDMA_ClearINT(rxDmaInitStruct->GDMA_Index, rxDmaInitStruct->GDMA_ChNum);
        UART_RXDMACmd(uartTable_[uart->index_].UARTx, DISABLE);
        uart->dataInFlight(true /* commit */, true /* cancel */);
        uart->receiving_ = false;
        uart->startReceiver();
        return 0;
    }

    void rxLock(bool lock) {
        if (rxDmaInitStruct_.GDMA_ChNum != 0xFF) {
            if (lock) {
                NVIC_DisableIRQ(GDMA_GetIrqNum(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum));
            } else {
                NVIC_EnableIRQ(GDMA_GetIrqNum(rxDmaInitStruct_.GDMA_Index, rxDmaInitStruct_.GDMA_ChNum));
            }
        }
    }

    void txLock(bool lock) {
        if (txDmaInitStruct_.GDMA_ChNum != 0xFF) {
            if (lock) {
                NVIC_DisableIRQ(GDMA_GetIrqNum(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum));
            } else {
                NVIC_EnableIRQ(GDMA_GetIrqNum(txDmaInitStruct_.GDMA_Index, txDmaInitStruct_.GDMA_ChNum));
            }
        }
    }


private:
    hal_pin_t txPin_;
    hal_pin_t rxPin_;
    hal_pin_t ctsPin_;
    hal_pin_t rtsPin_;

    GDMA_InitTypeDef txDmaInitStruct_;
    GDMA_InitTypeDef rxDmaInitStruct_;

    Config config_;

    particle::services::RingBuffer<uint8_t> txBuffer_;
    particle::services::RingBuffer<uint8_t> rxBuffer_;
    volatile size_t curTxCount_;

    volatile hal_usart_state_t state_;
    volatile bool transmitting_;
    volatile bool receiving_;
    bool configured_;

    uint8_t index_; // of UART_DEV_TABLE that is defined in rtl8721d_uart.c
    static constexpr const UART_DevTable* uartTable_ = UART_DEV_TABLE;

    static constexpr size_t MAX_DMA_BLOCK_SIZE = 4096;
    static constexpr size_t MAX_UART_FIFO_SIZE = 16;
};

int hal_usart_init_ex(hal_usart_interface_t serial, const hal_usart_buffer_config_t* config, void*) {
    auto usart = CHECK_TRUE_RETURN(Usart::getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);
    return usart->init(*config);
}

void hal_usart_init(hal_usart_interface_t serial, hal_usart_ring_buffer_t* rxBuffer, hal_usart_ring_buffer_t* txBuffer) {
    hal_usart_buffer_config_t conf = {
        .size = sizeof(hal_usart_buffer_config_t),
        .rx_buffer = rxBuffer->buffer,
        .rx_buffer_size = sizeof(rxBuffer->buffer),
        .tx_buffer = txBuffer->buffer,
        .tx_buffer_size = sizeof(txBuffer->buffer)
    };

    hal_usart_init_ex(serial, &conf, nullptr);
}

void hal_usart_begin_config(hal_usart_interface_t serial, uint32_t baud, uint32_t config, void*) {
    auto usart = Usart::getInstance(serial);
    if (!usart) {
        return;
    }
    Usart::Config conf = {
        .baudRate = baud,
        .config = config
    };
    usart->begin(conf);
}

void hal_usart_begin(hal_usart_interface_t serial, uint32_t baud) {
    hal_usart_begin_config(serial, baud, SERIAL_8N1, nullptr);
}

void hal_usart_end(hal_usart_interface_t serial) {
    auto usart = Usart::getInstance(serial);
    if (!usart) {
        return;
    }
    usart->end();
}

int32_t hal_usart_available_data_for_write(hal_usart_interface_t serial) {
    auto usart = CHECK_TRUE_RETURN(Usart::getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    return usart->space();
}

int32_t hal_usart_available(hal_usart_interface_t serial) {
    auto usart = CHECK_TRUE_RETURN(Usart::getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    return usart->data();
}

void hal_usart_flush(hal_usart_interface_t serial) {
    auto usart = Usart::getInstance(serial);
    if (!usart) {
        return;
    }
    usart->flush();
}

bool hal_usart_is_enabled(hal_usart_interface_t serial) {
    auto usart = CHECK_TRUE_RETURN(Usart::getInstance(serial), false);
    return usart->isEnabled();
}

ssize_t hal_usart_write_buffer(hal_usart_interface_t serial, const void* buffer, size_t size, size_t elementSize) {
    auto usart = CHECK_TRUE_RETURN(Usart::getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    CHECK_TRUE(elementSize == sizeof(uint8_t), SYSTEM_ERROR_INVALID_ARGUMENT);
    return usart->write((const uint8_t*)buffer, size);
}

ssize_t hal_usart_read_buffer(hal_usart_interface_t serial, void* buffer, size_t size, size_t elementSize) {
    auto usart = CHECK_TRUE_RETURN(Usart::getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    CHECK_TRUE(elementSize == sizeof(uint8_t), SYSTEM_ERROR_INVALID_ARGUMENT);
    return usart->read((uint8_t*)buffer, size);
}

ssize_t hal_usart_peek_buffer(hal_usart_interface_t serial, void* buffer, size_t size, size_t elementSize) {
    auto usart = CHECK_TRUE_RETURN(Usart::getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    CHECK_TRUE(elementSize == sizeof(uint8_t), SYSTEM_ERROR_INVALID_ARGUMENT);
    return usart->peek((uint8_t*)buffer, size);
}

int32_t hal_usart_read(hal_usart_interface_t serial) {
    auto usart = CHECK_TRUE_RETURN(Usart::getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    uint8_t c;
    CHECK(usart->read(&c, sizeof(c)));
    return c;
}

int32_t hal_usart_peek(hal_usart_interface_t serial) {
    auto usart = CHECK_TRUE_RETURN(Usart::getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    uint8_t c;
    CHECK(usart->peek(&c, sizeof(c)));
    return c;
}

uint32_t hal_usart_write_nine_bits(hal_usart_interface_t serial, uint16_t data) {
    return hal_usart_write(serial, data);
}

uint32_t hal_usart_write(hal_usart_interface_t serial, uint8_t data) {
    auto usart = CHECK_TRUE_RETURN(Usart::getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    // Blocking!
    while (usart->space() <= 0) {
        CHECK_RETURN(usart->pollStatus(), 0);
    }
    return CHECK_RETURN(usart->write(&data, sizeof(data)), 0);
}

void hal_usart_send_break(hal_usart_interface_t serial, void* reserved) {
    // Unsupported
}

uint8_t hal_usart_break_detected(hal_usart_interface_t serial) {
    // Unsupported
    return SYSTEM_ERROR_NONE;
}

void hal_usart_half_duplex(hal_usart_interface_t serial, bool enable) {
    // Unsupported
}

int hal_usart_pvt_get_event_group_handle(hal_usart_interface_t serial, EventGroupHandle_t* handle) {
    return SYSTEM_ERROR_NONE;
}

int hal_usart_pvt_enable_event(hal_usart_interface_t serial, HAL_USART_Pvt_Events events) {
    return 0;
}

int hal_usart_pvt_disable_event(hal_usart_interface_t serial, HAL_USART_Pvt_Events events) {
    return 0;
}

int hal_usart_pvt_wait_event(hal_usart_interface_t serial, uint32_t events, system_tick_t timeout) {
    return 0;
}

int hal_usart_sleep(hal_usart_interface_t serial, bool sleep, void* reserved) {
    return 0;
}
