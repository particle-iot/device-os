/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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
#include <nrf_uarte.h>
#include <nrf_ppi.h>
#include <nrf_timer.h>
#include "usart_hal.h"
#include "ringbuffer.h"
#include "pinmap_impl.h"
#include "check_nrf.h"
#include "gpio_hal.h"
#include <nrfx_prs.h>
#include <nrf_gpio.h>
#include <algorithm>
#include "hal_irq_flag.h"
#include "delay_hal.h"
#include "interrupts_hal.h"

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

class RxLock {
public:
    RxLock(NRF_UARTE_Type* uarte)
            : uarte_(uarte) {
        nrf_uarte_int_disable(uarte, NRF_UARTE_INT_ENDRX_MASK);
    }
    ~RxLock() {
        nrf_uarte_int_enable(uarte_, NRF_UARTE_INT_ENDRX_MASK);
    }

private:
    NRF_UARTE_Type* uarte_;
};

class TxLock {
public:
    TxLock(NRF_UARTE_Type* uarte)
            : uarte_(uarte) {
        nrf_uarte_int_disable(uarte, NRF_UARTE_INT_ENDTX_MASK);
    }
    ~TxLock() {
        nrf_uarte_int_enable(uarte_, NRF_UARTE_INT_ENDTX_MASK);
    }

private:
    NRF_UARTE_Type* uarte_;
};

inline uint32_t pinToNrf(pin_t pin) {
    const auto pinMap = HAL_Pin_Map();
    const auto& entry = pinMap[pin];
    return NRF_GPIO_PIN_MAP(entry.gpio_port, entry.gpio_pin);
}

class Usart;
Usart* getInstance(HAL_USART_Serial serial);
void uarte0InterruptHandler(void);
void uarte1InterruptHandler(void);

const uint8_t MAX_SCHEDULED_RECEIVALS = 1;
const size_t RESERVED_RX_SIZE = 0;
const size_t RX_THRESHOLD = 4;

class Usart {
public:
    Usart(NRF_UARTE_Type* instance, void (*interruptHandler)(void),
            app_irq_priority_t prio, NRF_TIMER_Type* timer,
            nrf_ppi_channel_t ppi, pin_t tx, pin_t rx, pin_t cts, pin_t rts)
            : uarte_(instance),
              interruptHandler_(interruptHandler),
              prio_(prio),
              timer_(timer),
              ppi_(ppi),
              txPin_(tx),
              rxPin_(rx),
              ctsPin_(cts),
              rtsPin_(rts),
              transmitting_(false),
              receiving_(0),
              rxConsumed_(0) {
    }

    struct Config {
        unsigned int baudRate;
        unsigned int config;
    };

    int init(const HAL_USART_Buffer_Config& conf) {
        if (isEnabled()) {
            CHECK(end());
        }

        if (isConfigured()) {
            CHECK(deInit());
        }

        CHECK_TRUE(conf.rx_buffer, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(conf.rx_buffer_size, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(conf.tx_buffer, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(conf.tx_buffer_size, SYSTEM_ERROR_INVALID_ARGUMENT);

        rxBuffer_.init((uint8_t*)conf.rx_buffer, conf.rx_buffer_size);
        txBuffer_.init((uint8_t*)conf.tx_buffer, conf.tx_buffer_size);

        configured_ = true;

        return 0;
    }

    int deInit() {
        configured_ = false;

        return 0;
    }

    int begin(const Config& conf) {
        CHECK_TRUE(isConfigured(), SYSTEM_ERROR_INVALID_STATE);
        CHECK_TRUE(validateConfig(conf.config), SYSTEM_ERROR_INVALID_ARGUMENT);
        auto nrfBaudRate = CHECK_RETURN(getNrfBaudrate(conf.baudRate), SYSTEM_ERROR_INVALID_ARGUMENT);

        AtomicSection lk;

        if (isEnabled()) {
            end();
        }

        nrfx_prs_acquire(uarte_, interruptHandler_);

        HAL_GPIO_Write(txPin_, 1);
        HAL_Pin_Mode(txPin_, OUTPUT);
        HAL_Pin_Mode(rxPin_, INPUT);
        HAL_Set_Pin_Function(txPin_, PF_UART);
        HAL_Set_Pin_Function(rxPin_, PF_UART);

        nrf_uarte_baudrate_set(uarte_, (nrf_uarte_baudrate_t)nrfBaudRate);
        nrf_uarte_configure(uarte_,
                conf.config & SERIAL_PARITY_EVEN ? NRF_UARTE_PARITY_INCLUDED : NRF_UARTE_PARITY_EXCLUDED,
                conf.config & SERIAL_FLOW_CONTROL_RTS_CTS ? NRF_UARTE_HWFC_ENABLED : NRF_UARTE_HWFC_DISABLED);
        nrf_uarte_txrx_pins_set(uarte_, pinToNrf(txPin_), pinToNrf(rxPin_));

        if (conf.config & SERIAL_FLOW_CONTROL_CTS) {
            HAL_Pin_Mode(ctsPin_, INPUT);
            HAL_Set_Pin_Function(ctsPin_, PF_UART);
        }

        if (conf.config & SERIAL_FLOW_CONTROL_RTS) {
            HAL_GPIO_Write(rtsPin_, 1);
            HAL_Pin_Mode(rtsPin_, OUTPUT);
            HAL_Set_Pin_Function(rtsPin_, PF_UART);
        }

        if (conf.config & SERIAL_FLOW_CONTROL_RTS_CTS) {
            uint32_t cts = NRF_UARTE_PSEL_DISCONNECTED;
            uint32_t rts = NRF_UARTE_PSEL_DISCONNECTED;
            if (conf.config & SERIAL_FLOW_CONTROL_CTS) {
                cts = pinToNrf(ctsPin_);
            }
            if (conf.config & SERIAL_FLOW_CONTROL_RTS) {
                rts = pinToNrf(rtsPin_);
            }
            nrf_uarte_hwfc_pins_set(uarte_, rts, cts);
        }

        nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ERROR);

        nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_RXSTARTED);
        nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_RXDRDY);
        nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_RXTO);
        nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ENDRX);

        nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_TXSTARTED);
        nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_TXDRDY);
        nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_TXSTOPPED);
        nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ENDTX);

        disableInterrupts();

        nrf_uarte_int_enable(uarte_, NRF_UARTE_INT_ENDRX_MASK | NRF_UARTE_INT_ENDTX_MASK);

        NRFX_IRQ_PRIORITY_SET(nrfx_get_irq_number((void *)uarte_), prio_);
        NRFX_IRQ_ENABLE(nrfx_get_irq_number((void *)uarte_));

        nrf_uarte_enable(uarte_);

        config_ = conf;
        enabled_ = true;

        enableTimer();
        startReceiver();

        return 0;
    }

    int end() {
        CHECK_TRUE(enabled_, SYSTEM_ERROR_INVALID_STATE);
        AtomicSection lk;

        disableInterrupts();

        stopTransmission();
        stopReceiver();

        nrf_uarte_disable(uarte_);

        nrf_uarte_txrx_pins_disconnect(uarte_);
        nrf_uarte_hwfc_pins_disconnect(uarte_);

        HAL_Pin_Mode(txPin_, INPUT);
        HAL_Set_Pin_Function(txPin_, PF_NONE);
        HAL_Pin_Mode(rxPin_, INPUT);
        HAL_Set_Pin_Function(rxPin_, PF_NONE);

        if (config_.config & SERIAL_FLOW_CONTROL_CTS) {
            HAL_Pin_Mode(ctsPin_, INPUT);
            HAL_Set_Pin_Function(ctsPin_, PF_NONE);
        }
        if (config_.config & SERIAL_FLOW_CONTROL_RTS) {
            HAL_Pin_Mode(rtsPin_, INPUT);
            HAL_Set_Pin_Function(rtsPin_, PF_NONE);
        }

        nrfx_prs_release(uarte_);

        disableTimer();

        enabled_ = false;
        config_ = {};
        transmitting_ = false;
        receiving_ = 0;
        rxBuffer_.reset();
        txBuffer_.reset();

        return 0;
    }

    ssize_t data() {
        CHECK_TRUE(isEnabled(), SYSTEM_ERROR_INVALID_STATE);
        RxLock lk(uarte_);
        ssize_t d = rxBuffer_.data();
        if (receiving_) {
            const ssize_t toConsume = timerValue() - rxConsumed_;
            if (toConsume > 0) {
                rxBuffer_.acquireCommit(toConsume);
                rxConsumed_ += toConsume;
                d += toConsume;
            }
        }
        return d;
    }

    ssize_t space() {
        CHECK_TRUE(isEnabled(), SYSTEM_ERROR_INVALID_STATE);
        TxLock lk(uarte_);
        return txBuffer_.space();
    }

    ssize_t read(uint8_t* buffer, size_t size) {
        CHECK_TRUE(isEnabled(), SYSTEM_ERROR_INVALID_STATE);
        const ssize_t maxRead = CHECK(data());
        const size_t readSize = std::min((size_t)maxRead, size);
        CHECK_TRUE(readSize > 0, SYSTEM_ERROR_NO_MEMORY);
        ssize_t r;
        {
            RxLock lk(uarte_);
            r = CHECK(rxBuffer_.get(buffer, readSize));
        }
        if (!receiving_) {
            startReceiver();
        }
        return r;
    }

    ssize_t peek(uint8_t* buffer, size_t size) {
        CHECK_TRUE(isEnabled(), SYSTEM_ERROR_INVALID_STATE);
        const ssize_t maxRead = CHECK(data());
        const size_t peekSize = std::min((size_t)maxRead, size);
        CHECK_TRUE(peekSize > 0, SYSTEM_ERROR_NO_MEMORY);
        RxLock lk(uarte_);
        return rxBuffer_.peek(buffer, peekSize);
    }

    ssize_t write(const uint8_t* buffer, size_t size) {
        CHECK_TRUE(isEnabled(), SYSTEM_ERROR_INVALID_STATE);
        const ssize_t canWrite = CHECK(space());
        const size_t writeSize = std::min((size_t)canWrite, size);
        CHECK_TRUE(writeSize > 0, SYSTEM_ERROR_NO_MEMORY);
        ssize_t r;
        {
            TxLock lk(uarte_);
            r = CHECK(txBuffer_.put(buffer, writeSize));
        }
        // Start transmission
        startTransmission();
        return r;
    }

    ssize_t flush() {
        while (true) {
            while (transmitting_) {
                // FIXME: busy loop
            }
            {
                TxLock lk(uarte_);
                if (!enabled_ || txBuffer_.empty()) {
                    break;
                }
            }
        }
        return 0;
    }

    bool isConfigured() const {
        return configured_;
    }

    bool isEnabled() const {
        return enabled_;
    }

    void pump() {
        if (!willPreempt() && isEnabled()) {
            interruptHandler();
        }
    }

    void interruptHandler() {
        if (nrf_uarte_event_check(uarte_, NRF_UARTE_EVENT_ENDRX)) {
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ENDRX);
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_RXDRDY);

            nrf_timer_task_trigger(timer_, NRF_TIMER_TASK_CLEAR);
            rxConsumed_ = 0;

            if (rxBuffer_.acquirePending() > 0) {
                rxBuffer_.acquireCommit(rxBuffer_.acquirePending());
            }

            --receiving_;
            startReceiver();
            return;
        }
        if (nrf_uarte_event_check(uarte_, NRF_UARTE_EVENT_ENDTX)) {
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ENDTX);
            txBuffer_.consumeCommit(nrf_uarte_tx_amount_get(uarte_));
            transmitting_ = false;
            startTransmission();
        }
    }

private:
    void startTransmission() {
        size_t consumable;
        if (!transmitting_ && (consumable = txBuffer_.consumable())) {
            transmitting_ = true;
            auto ptr = txBuffer_.consume(consumable);
#ifdef DEBUG_BUILD
            SPARK_ASSERT(ptr);
#endif // DEBUG_BUILD
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_TXDRDY);
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ENDTX);
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_TXSTOPPED);
            nrf_uarte_tx_buffer_set(uarte_, ptr, consumable);
            nrf_uarte_task_trigger(uarte_, NRF_UARTE_TASK_STARTTX);
        }
    }

    void startReceiver(bool flush = false) {
        if (receiving_ >= MAX_SCHEDULED_RECEIVALS) {
            return;
        }

        // Updates current size
        rxBuffer_.acquireBegin();

        const size_t acquirable = rxBuffer_.acquirable();
        const size_t acquirableWrapped = rxBuffer_.acquirableWrapped();
        size_t rxSize = std::max(acquirable, acquirableWrapped);

        if (rxSize < RX_THRESHOLD) {
            return;
        }

        if (RESERVED_RX_SIZE) {
            if (!flush) {
                // We should always reserve RESERVED_RX_SIZE bytes in the rx buffer
                if (rxSize == acquirable) {
                    if (acquirableWrapped < RESERVED_RX_SIZE) {
                        if (rxSize > RESERVED_RX_SIZE) {
                            rxSize -= RESERVED_RX_SIZE;
                        } else {
                            return;
                        }
                    }
                } else {
                    if (rxSize > RESERVED_RX_SIZE) {
                        rxSize -= RESERVED_RX_SIZE;
                    } else {
                        return;
                    }
                }
            } else {
                rxSize = std::min(RESERVED_RX_SIZE, rxSize);
            }
        }

        if (rxSize > 0) {
            ++receiving_;
            auto ptr = rxBuffer_.acquire(rxSize);
#ifdef DEBUG_BUILD
            SPARK_ASSERT(ptr);
#endif // DEBUG_BUILD
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_RXDRDY);
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ENDRX);
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_RXTO);
            nrf_uarte_rx_buffer_set(uarte_, ptr, rxSize);
            if (!flush) {
                nrf_uarte_task_trigger(uarte_, NRF_UARTE_TASK_STARTRX);
            } else {
                nrf_uarte_task_trigger(uarte_, NRF_UARTE_TASK_FLUSHRX);
            }
        }
    }

    void stopReceiver() {
        if (receiving_) {
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_RXTO);
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ENDRX);
            nrf_uarte_task_trigger(uarte_, NRF_UARTE_TASK_STOPRX);
            bool rxto = false;
            bool endrx = false;
            while (!(rxto && endrx)) {
                if (nrf_uarte_event_check(uarte_, NRF_UARTE_EVENT_ENDRX)) {
                    nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ENDRX);
                    endrx = true;
                }
                if (nrf_uarte_event_check(uarte_, NRF_UARTE_EVENT_RXTO)) {
                    nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_RXTO);
                    rxto = true;
                }
            }
        }
    }

    void stopTransmission() {
        if (transmitting_) {
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_TXSTOPPED);
            nrf_uarte_task_trigger(uarte_, NRF_UARTE_TASK_STOPTX);
            while (!nrf_uarte_event_check(uarte_, NRF_UARTE_EVENT_TXSTOPPED));
        }
    }

    void disableInterrupts() {
        nrf_uarte_int_disable(uarte_, NRF_UARTE_INT_ERROR_MASK |
                      NRF_UARTE_INT_RXSTARTED_MASK |
                      NRF_UARTE_INT_RXDRDY_MASK |
                      NRF_UARTE_INT_RXTO_MASK |
                      NRF_UARTE_INT_ENDRX_MASK |
                      NRF_UARTE_INT_TXSTARTED_MASK |
                      NRF_UARTE_INT_TXDRDY_MASK |
                      NRF_UARTE_INT_TXSTOPPED_MASK |
                      NRF_UARTE_INT_ENDTX_MASK);
        NRFX_IRQ_DISABLE(nrfx_get_irq_number((void*)uarte_));
    }

    void enableTimer() {
        NRFX_IRQ_DISABLE(nrfx_get_irq_number((void*)timer_));
        nrf_timer_mode_set(timer_, NRF_TIMER_MODE_COUNTER);
        nrf_timer_bit_width_set(timer_, NRF_TIMER_BIT_WIDTH_16);
        nrf_timer_task_trigger(timer_, NRF_TIMER_TASK_CLEAR);
        nrf_timer_task_trigger(timer_, NRF_TIMER_TASK_START);

        nrf_ppi_channel_endpoint_setup(ppi_, (uint32_t)&uarte_->EVENTS_RXDRDY,
                (uint32_t)&timer_->TASKS_COUNT);
        nrf_ppi_channel_enable(ppi_);
    }

    void disableTimer() {
        nrf_timer_task_trigger(timer_, NRF_TIMER_TASK_CLEAR);
        nrf_timer_task_trigger(timer_, NRF_TIMER_TASK_SHUTDOWN);
        nrf_ppi_channel_disable(ppi_);
        rxConsumed_ = 0;
    }

    size_t timerValue() {
        nrf_timer_task_trigger(timer_, NRF_TIMER_TASK_CAPTURE0);
        return nrf_timer_cc_read(timer_, NRF_TIMER_CC_CHANNEL0);
    }

    bool willPreempt() const {
        // Check if interrupts are disabled:
        // 1. Globally
        // 2. Application interrupts through SoftDevice
        // 3. Via BASEPRI
        if ((__get_PRIMASK() & 1) || nrf_nvic_state.__cr_flag || __get_BASEPRI() >= prio_) {
            return false;
        }

        if (HAL_IsISR()) {
            if (!HAL_WillPreempt(nrfx_get_irq_number((void*)uarte_), HAL_ServicedIRQn())) {
                return false;
            }
        }

        return true;
    }

    static bool validateConfig(unsigned int config) {
        CHECK_TRUE((config & SERIAL_MODE) == SERIAL_8N1 ||
                   (config & SERIAL_MODE) == SERIAL_8E1 ||
                   (config & SERIAL_MODE) == SERIAL_8N2 ||
                   (config & SERIAL_MODE) == SERIAL_8E2, false);

        CHECK_TRUE((config & SERIAL_HALF_DUPLEX) == 0, false);
        CHECK_TRUE((config & LIN_MODE) == 0, false);

        // LIN-mode configuration and half-duplex pin configuration is ignored

        return true;
    }

    struct BaudrateMap {
        unsigned int baudRate;
        nrf_uarte_baudrate_t nrfBaudRate;
    };

    static int getNrfBaudrate(unsigned int baudRate) {
        for (unsigned int i = 0; i < sizeof(baudrateMap_) / sizeof(baudrateMap_[0]); i++) {
            if (baudrateMap_[i].baudRate == baudRate) {
                return baudrateMap_[i].nrfBaudRate;
            }
        }

        return SYSTEM_ERROR_NOT_FOUND;
    }

private:
    static constexpr const BaudrateMap baudrateMap_[] = {
        { 1200,     NRF_UARTE_BAUDRATE_1200    },
        { 2400,     NRF_UARTE_BAUDRATE_2400    },
        { 4800,     NRF_UARTE_BAUDRATE_4800    },
        { 9600,     NRF_UARTE_BAUDRATE_9600    },
        { 14400,    NRF_UARTE_BAUDRATE_14400   },
        { 19200,    NRF_UARTE_BAUDRATE_19200   },
        { 28800,    NRF_UARTE_BAUDRATE_28800   },
        { 38400,    NRF_UARTE_BAUDRATE_38400   },
        { 57600,    NRF_UARTE_BAUDRATE_57600   },
        { 76800,    NRF_UARTE_BAUDRATE_76800   },
        { 115200,   NRF_UARTE_BAUDRATE_115200  },
        { 230400,   NRF_UARTE_BAUDRATE_230400  },
        { 250000,   NRF_UARTE_BAUDRATE_250000  },
        { 460800,   NRF_UARTE_BAUDRATE_460800  },
        { 921600,   NRF_UARTE_BAUDRATE_921600  },
        { 1000000,  NRF_UARTE_BAUDRATE_1000000 }
    };

    NRF_UARTE_Type* uarte_;
    void (*interruptHandler_)(void);
    app_irq_priority_t prio_;
    NRF_TIMER_Type* timer_;
    nrf_ppi_channel_t ppi_;

    pin_t txPin_;
    pin_t rxPin_;
    pin_t ctsPin_;
    pin_t rtsPin_;

    bool configured_ = false;
    bool enabled_ = false;

    volatile bool transmitting_;
    volatile uint8_t receiving_;
    volatile size_t rxConsumed_;

    Config config_ = {};

    particle::services::RingBuffer<uint8_t> txBuffer_;
    particle::services::RingBuffer<uint8_t> rxBuffer_;
};

constexpr const Usart::BaudrateMap Usart::baudrateMap_[];

const auto UARTE0_INTERRUPT_PRIORITY = APP_IRQ_PRIORITY_LOW;
// TODO: move this to hal_platform_config.h ?
#if PLATFORM_ID == PLATFORM_XENON || PLATFORM_ID == PLATFORM_XSOM
const auto UARTE1_INTERRUPT_PRIORITY = APP_IRQ_PRIORITY_LOW;
#else
const auto UARTE1_INTERRUPT_PRIORITY = (app_irq_priority_t)_PRIO_SD_LOWEST;
#endif // PLATFORM_ID == PLATFORM_XENON || PLATFORM_ID == PLATFORM_XSOM

Usart* getInstance(HAL_USART_Serial serial) {
    static Usart usartMap[] = {
        {NRF_UARTE0, uarte0InterruptHandler, UARTE0_INTERRUPT_PRIORITY, NRF_TIMER3, NRF_PPI_CHANNEL4, TX, RX, CTS, RTS},
        {NRF_UARTE1, uarte1InterruptHandler, UARTE1_INTERRUPT_PRIORITY, NRF_TIMER4, NRF_PPI_CHANNEL5, TX1, RX1, CTS1, RTS1}
    };

    CHECK_TRUE(serial < sizeof(usartMap) / sizeof(usartMap[0]), nullptr);

    return &usartMap[serial];
}

void uarte0InterruptHandler(void) {
    getInstance(HAL_USART_SERIAL1)->interruptHandler();
}

void uarte1InterruptHandler(void) {
    getInstance(HAL_USART_SERIAL2)->interruptHandler();
}

} // anonymous

extern "C" void UARTE1_IRQHandler(void) {
    uarte1InterruptHandler();
}

int HAL_USART_Init_Ex(HAL_USART_Serial serial, const HAL_USART_Buffer_Config* config, void*) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);

    return usart->init(*config);
}

void HAL_USART_Init(HAL_USART_Serial serial, Ring_Buffer* rxBuffer, Ring_Buffer* txBuffer) {
    HAL_USART_Buffer_Config conf = {
        .size = sizeof(HAL_USART_Buffer_Config),
        .rx_buffer = rxBuffer->buffer,
        .rx_buffer_size = sizeof(rxBuffer->buffer),
        .tx_buffer = txBuffer->buffer,
        .tx_buffer_size = sizeof(txBuffer->buffer)
    };

    HAL_USART_Init_Ex(serial, &conf, nullptr);
}

void HAL_USART_BeginConfig(HAL_USART_Serial serial, uint32_t baud, uint32_t config, void*) {
    auto usart = getInstance(serial);
    // FIXME: CHECK_XXX that doesn't return anything?
    if (!usart) {
        return;
    }

    Usart::Config conf = {
        .baudRate = baud,
        .config = config
    };

    usart->begin(conf);
}

void HAL_USART_Begin(HAL_USART_Serial serial, uint32_t baud) {
    HAL_USART_BeginConfig(serial, baud, SERIAL_8N1, nullptr);
}

void HAL_USART_End(HAL_USART_Serial serial) {
    auto usart = getInstance(serial);
    // FIXME: CHECK_XXX?
    if (!usart) {
        return;
    }

    usart->end();
}

int32_t HAL_USART_Available_Data_For_Write(HAL_USART_Serial serial) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    return usart->space();
}

int32_t HAL_USART_Available_Data(HAL_USART_Serial serial) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    return usart->data();
}

void HAL_USART_Flush_Data(HAL_USART_Serial serial) {
    auto usart = getInstance(serial);
    // FIXME: CHECK_XXX?
    if (!usart) {
        return;
    }

    usart->flush();
}

bool HAL_USART_Is_Enabled(HAL_USART_Serial serial) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), false);

    return usart->isEnabled();
}

ssize_t HAL_USART_Write(HAL_USART_Serial serial, const void* buffer, size_t size, size_t elementSize) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    CHECK_TRUE(elementSize == sizeof(uint8_t), SYSTEM_ERROR_INVALID_ARGUMENT);
    usart->pump();
    return usart->write((const uint8_t*)buffer, size);
}

ssize_t HAL_USART_Read(HAL_USART_Serial serial, void* buffer, size_t size, size_t elementSize) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    CHECK_TRUE(elementSize == sizeof(uint8_t), SYSTEM_ERROR_INVALID_ARGUMENT);
    return usart->read((uint8_t*)buffer, size);
}

ssize_t HAL_USART_Peek(HAL_USART_Serial serial, void* buffer, size_t size, size_t elementSize) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    CHECK_TRUE(elementSize == sizeof(uint8_t), SYSTEM_ERROR_INVALID_ARGUMENT);
    return usart->peek((uint8_t*)buffer, size);
}

int32_t HAL_USART_Read_Data(HAL_USART_Serial serial) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    uint8_t c;
    CHECK(usart->read(&c, sizeof(c)));
    return c;
}

int32_t HAL_USART_Peek_Data(HAL_USART_Serial serial) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    uint8_t c;
    CHECK(usart->peek(&c, sizeof(c)));
    return c;
}

uint32_t HAL_USART_Write_NineBitData(HAL_USART_Serial serial, uint16_t data) {
    return HAL_USART_Write_Data(serial, data);
}

uint32_t HAL_USART_Write_Data(HAL_USART_Serial serial, uint8_t data) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    // Blocking!
    while(usart->isEnabled() && usart->space() <= 0) {
        usart->pump();
    }
    return CHECK_RETURN(usart->write(&data, sizeof(data)), 0);
}

void HAL_USART_Send_Break(HAL_USART_Serial serial, void* reserved) {
    // Unsupported
}

uint8_t HAL_USART_Break_Detected(HAL_USART_Serial serial) {
    // Unsupported
    return 0;
}

void HAL_USART_Half_Duplex(HAL_USART_Serial serial, bool enable) {
    // Unsupported
}
