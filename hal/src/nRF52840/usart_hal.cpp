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
#include <nrf_uarte.h>
#include <nrf_ppi.h>
#include <nrf_timer.h>
#include "usart_hal.h"
#include "ringbuffer.h"
#include "pinmap_hal.h"
#include "check_nrf.h"
#include "gpio_hal.h"
#include <nrfx_prs.h>
#include <nrf_gpio.h>
#include <algorithm>
#include "hal_irq_flag.h"
#include "delay_hal.h"
#include "interrupts_hal.h"
#include "usart_hal_private.h"
#include "timer_hal.h"

#if PLATFORM_ID == PLATFORM_TRACKER
#include "i2c_hal.h"
#endif

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

inline uint32_t pinToNrf(hal_pin_t pin) {
    const auto pinMap = hal_pin_map();
    const auto& entry = pinMap[pin];
    return NRF_GPIO_PIN_MAP(entry.gpio_port, entry.gpio_pin);
}

class Usart;
Usart* getInstance(hal_usart_interface_t serial);
void uarte0InterruptHandler(void);
void uarte1InterruptHandler(void);

const uint8_t MAX_SCHEDULED_RECEIVALS = 1;
const size_t RESERVED_RX_SIZE = 0;
const size_t RX_THRESHOLD = 4;

class Usart {
public:
    Usart(NRF_UARTE_Type* instance, void (*interruptHandler)(void),
            app_irq_priority_t prio, NRF_TIMER_Type* timer,
            nrf_ppi_channel_t ppi, hal_pin_t tx, hal_pin_t rx, hal_pin_t cts, hal_pin_t rts)
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
              rxConsumed_(0),
              evGroup_(nullptr) {
        evGroup_ = xEventGroupCreate();
        SPARK_ASSERT(evGroup_);
    }

    struct Config {
        unsigned int baudRate;
        unsigned int config;
    };

    int init(const hal_usart_buffer_config_t& conf) {
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

        return SYSTEM_ERROR_NONE;
    }

    int deInit() {
        configured_ = false;

        return SYSTEM_ERROR_NONE;
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

        hal_gpio_write(txPin_, 1);
        hal_gpio_mode(txPin_, OUTPUT);
        hal_gpio_mode(rxPin_, INPUT);
        hal_pin_set_function(txPin_, PF_UART);
        hal_pin_set_function(rxPin_, PF_UART);

        nrf_uarte_baudrate_set(uarte_, (nrf_uarte_baudrate_t)nrfBaudRate);
        nrf_uarte_configure(uarte_,
                conf.config & SERIAL_PARITY_EVEN ? NRF_UARTE_PARITY_INCLUDED : NRF_UARTE_PARITY_EXCLUDED,
                conf.config & SERIAL_FLOW_CONTROL_RTS_CTS ? NRF_UARTE_HWFC_ENABLED : NRF_UARTE_HWFC_DISABLED);
        nrf_uarte_txrx_pins_set(uarte_, pinToNrf(txPin_), pinToNrf(rxPin_));

        if (conf.config & SERIAL_FLOW_CONTROL_CTS) {
            hal_gpio_mode(ctsPin_, INPUT);
            hal_pin_set_function(ctsPin_, PF_UART);
        }

        if (conf.config & SERIAL_FLOW_CONTROL_RTS) {
            hal_gpio_write(rtsPin_, 1);
            hal_gpio_mode(rtsPin_, OUTPUT);
            hal_pin_set_function(rtsPin_, PF_UART);
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
        state_ = HAL_USART_STATE_ENABLED;

        enableTimer();
        startReceiver();

        return SYSTEM_ERROR_NONE;
    }

    int end() {
        CHECK_TRUE(state_ != HAL_USART_STATE_DISABLED, SYSTEM_ERROR_INVALID_STATE);
        AtomicSection lk;

        CHECK(disable());

        state_ = HAL_USART_STATE_DISABLED;
        transmitting_ = false;
        receiving_ = 0;
        config_ = {};
        rxBuffer_.reset();
        txBuffer_.reset();

        return SYSTEM_ERROR_NONE;
    }

    int suspend() {
        CHECK_TRUE(isEnabled(), SYSTEM_ERROR_INVALID_STATE);
        
        flush();

        AtomicSection lk;

        // Update current available received data
        data();
        CHECK(disable(false));

        state_ = HAL_USART_STATE_SUSPENDED;
        transmitting_ = false;
        receiving_ = 0;
        rxBuffer_.prune();
        txBuffer_.reset();

        return SYSTEM_ERROR_NONE;
    }

    int restore() {
        AtomicSection lk;
        CHECK_TRUE(state_ == HAL_USART_STATE_SUSPENDED, SYSTEM_ERROR_INVALID_STATE);
        return begin(config_);
    }

    ssize_t data() {
        CHECK_TRUE(isEnabled(), SYSTEM_ERROR_INVALID_STATE);
        RxLock lk(uarte_);
        ssize_t d = rxBuffer_.data();
        if (receiving_) {
            // IMPORTANT: It seems that the timer value (which is a counter for every time UARTE generates RXDRDY event)
            // may potentially get larger than the configured RX DMA transfer:
            // <quote>
            // For each byte received over the RXD line, an RXDRDY event will be generated.
            // This event is likely to occur before the corresponding data has been transferred to Data RAM.
            // </quote>
            // We'll be extra careful here and make sure not to consume more than we can, otherwise
            // we may put the ring buffer into an invalid state as there are not safety checks.
            const ssize_t toConsume = std::min(rxBuffer_.acquirePending(), timerValue() - rxConsumed_);
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
        RxLock lk(uarte_);
        ssize_t r = CHECK(rxBuffer_.get(buffer, readSize));
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
                if (!isEnabled() || txBuffer_.empty()) {
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
        return state_ == HAL_USART_STATE_ENABLED;
    }

    void pump() {
        if (!willPreempt() && isEnabled()) {
            interruptHandler();
        }
    }

    void interruptHandler() {
        BaseType_t yield = pdFALSE;
        bool eventGenerated = false;

        if (nrf_uarte_int_enable_check(uarte_, NRF_UARTE_INT_RXDRDY_MASK)) {
            if (nrf_uarte_event_check(uarte_, NRF_UARTE_EVENT_RXDRDY)) {
                nrf_uarte_int_disable(uarte_, NRF_UARTE_INT_RXDRDY_MASK);
                nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_RXDRDY);
                if (xEventGroupSetBitsFromISR(evGroup_, HAL_USART_PVT_EVENT_READABLE, &yield) != pdFAIL) {
                    eventGenerated = true;
                }
            }
        }

        if (nrf_uarte_int_enable_check(uarte_, NRF_UARTE_INT_TXDRDY_MASK)) {
            if (nrf_uarte_event_check(uarte_, NRF_UARTE_EVENT_TXDRDY)) {
                nrf_uarte_int_disable(uarte_, NRF_UARTE_INT_TXDRDY_MASK);
                nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_TXDRDY);
                BaseType_t yieldTx = pdFALSE;
                if (xEventGroupSetBitsFromISR(evGroup_, HAL_USART_PVT_EVENT_WRITABLE, &yieldTx) != pdFAIL) {
                    eventGenerated = true;
                }
                yield = yield || yieldTx;
            }
        }

        // IMPORTANT: we cannot process ENDRX event if we are under the RX lock (ENDRX interrupt is disabled)
        if (nrf_uarte_int_enable_check(uarte_, NRF_UARTE_INT_ENDRX_MASK)) {
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
            }
        }

        // IMPORTANT: we cannot process ENDTX event if we are under the TX lock (ENDTX interrupt is disabled)
        if (nrf_uarte_int_enable_check(uarte_, NRF_UARTE_INT_ENDTX_MASK)) {
            if (nrf_uarte_event_check(uarte_, NRF_UARTE_EVENT_ENDTX)) {
                nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ENDTX);
                nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_TXDRDY);
                txBuffer_.consumeCommit(nrf_uarte_tx_amount_get(uarte_));
                transmitting_ = false;
                startTransmission();
            }
        }
        if (nrf_uarte_event_check(uarte_, NRF_UARTE_EVENT_ERROR)) {
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ERROR);
            uint32_t uartErrorSource = nrf_uarte_errorsrc_get_and_clear(uarte_);
            (void)uartErrorSource;
        }

        if (eventGenerated) {
            portYIELD_FROM_ISR(yield);
        }
    }

    EventGroupHandle_t eventGroup() {
        return evGroup_;
    }

    int enableEvent(HAL_USART_Pvt_Events event) {
        if (event & HAL_USART_PVT_EVENT_READABLE) {
            if (data() <= 0) {
                nrf_uarte_int_disable(uarte_, NRF_UARTE_INT_RXDRDY_MASK);
                nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_RXDRDY);
                nrf_uarte_int_enable(uarte_, NRF_UARTE_INT_RXDRDY_MASK);
            } else {
                xEventGroupSetBits(evGroup_, HAL_USART_PVT_EVENT_READABLE);
            }
        }

        if (event & HAL_USART_PVT_EVENT_WRITABLE) {
            if (space() <= 0) {
                nrf_uarte_int_disable(uarte_, NRF_UARTE_INT_TXDRDY_MASK);
                nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_TXDRDY);
                nrf_uarte_int_enable(uarte_, NRF_UARTE_INT_TXDRDY_MASK);
            } else {
                xEventGroupSetBits(evGroup_, HAL_USART_PVT_EVENT_WRITABLE);
            }
        }

        return SYSTEM_ERROR_NONE;
    }

    int disableEvent(HAL_USART_Pvt_Events event) {
        if (event & HAL_USART_PVT_EVENT_READABLE) {
            nrf_uarte_int_disable(uarte_, NRF_UARTE_INT_RXDRDY_MASK);
            xEventGroupClearBits(evGroup_, HAL_USART_PVT_EVENT_READABLE);
        }

        if (event & HAL_USART_PVT_EVENT_WRITABLE) {
            nrf_uarte_int_disable(uarte_, NRF_UARTE_INT_TXDRDY_MASK);
            xEventGroupClearBits(evGroup_, HAL_USART_PVT_EVENT_WRITABLE);
        }

        return SYSTEM_ERROR_NONE;
    }

    int waitEvent(uint32_t events, system_tick_t timeout) {
        CHECK(enableEvent((HAL_USART_Pvt_Events)events));

        auto res = xEventGroupWaitBits(evGroup_, events, pdTRUE, pdFALSE, timeout / portTICK_RATE_MS);
        return res;
    }

private:
    int disable(bool end = true) {
        CHECK_TRUE(isEnabled(), SYSTEM_ERROR_INVALID_STATE);

        if (config_.config & SERIAL_FLOW_CONTROL_RTS) {
            if (receiving_) {
                // Issue the STOPRX task to deactive the RTS
                nrf_uarte_task_trigger(uarte_, NRF_UARTE_TASK_STOPRX);
            }
        }

        disableInterrupts();
        stopTransmission();
        stopReceiver();

        nrf_uarte_disable(uarte_);
        nrf_uarte_txrx_pins_disconnect(uarte_);
        nrf_uarte_hwfc_pins_disconnect(uarte_);
        nrfx_prs_release(uarte_);

        disableTimer();

        // Configuring the input pins' mode to PIN_MODE_NONE will disconnect input buffer to enable power savings
        // Configuring the output pins' mode to INPUT_PULLUP to not polluting the state on other side.
        hal_gpio_mode(txPin_, end ? PIN_MODE_NONE : INPUT_PULLUP);
        hal_gpio_mode(rxPin_, PIN_MODE_NONE);
        if (config_.config & SERIAL_FLOW_CONTROL_RTS) {
            hal_gpio_mode(rtsPin_, end ? PIN_MODE_NONE : INPUT_PULLUP);
        }
        if (config_.config & SERIAL_FLOW_CONTROL_CTS) {
            hal_gpio_mode(ctsPin_, PIN_MODE_NONE);
        }
        if (end) {
            hal_pin_set_function(txPin_, PF_NONE);
            hal_pin_set_function(rxPin_, PF_NONE);
            if (config_.config & SERIAL_FLOW_CONTROL_RTS) {
                hal_pin_set_function(rtsPin_, PF_NONE);
            }
            if (config_.config & SERIAL_FLOW_CONTROL_CTS) {
                hal_pin_set_function(ctsPin_, PF_NONE);
            }
        }

        return SYSTEM_ERROR_NONE;
    }

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
        NRFX_IRQ_PENDING_CLEAR(nrfx_get_irq_number((void*)uarte_));
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
        if (hal_interrupt_is_irq_masked(nrfx_get_irq_number((void*)uarte_)) || nrf_nvic_state.__cr_flag) {
            return false;
        }

        if (hal_interrupt_is_isr()) {
            if (!hal_interrupt_will_preempt(nrfx_get_irq_number((void*)uarte_), hal_interrupt_serviced_irqn())) {
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

    hal_pin_t txPin_;
    hal_pin_t rxPin_;
    hal_pin_t ctsPin_;
    hal_pin_t rtsPin_;

    bool configured_ = false;
    volatile hal_usart_state_t state_ = HAL_USART_STATE_DISABLED;

    volatile bool transmitting_;
    volatile uint8_t receiving_;
    volatile size_t rxConsumed_;

    Config config_ = {};

    particle::services::RingBuffer<uint8_t> txBuffer_;
    particle::services::RingBuffer<uint8_t> rxBuffer_;

    EventGroupHandle_t evGroup_;
};

constexpr const Usart::BaudrateMap Usart::baudrateMap_[];

const auto UARTE0_INTERRUPT_PRIORITY = APP_IRQ_PRIORITY_LOW;
// TODO: move this to hal_platform_config.h ?
const auto UARTE1_INTERRUPT_PRIORITY = (app_irq_priority_t)_PRIO_SD_LOWEST;

Usart* getInstance(hal_usart_interface_t serial) {
    static Usart usartMap[] = {
        // NOTE: NCP should be last so that Serial1, Serial2, etc.. are contiguous.
        {NRF_UARTE0, uarte0InterruptHandler, UARTE0_INTERRUPT_PRIORITY, NRF_TIMER2, NRF_PPI_CHANNEL4, TX, RX, CTS, RTS},    // Serial1
        {NRF_UARTE1, uarte1InterruptHandler, UARTE1_INTERRUPT_PRIORITY, NRF_TIMER3, NRF_PPI_CHANNEL5, TX1, RX1, CTS1, RTS1} // NCP
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

int hal_usart_init_ex(hal_usart_interface_t serial, const hal_usart_buffer_config_t* config, void*) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
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
#if PLATFORM_ID == PLATFORM_TRACKER
    /*
     * On Tracker both I2C_INTERFACE3 and USART_SERIAL1 use the same pins - D8 and D9,
     * We cannot enable both of them at the same time.
     */
    if (serial == HAL_USART_SERIAL1) {
        if (hal_i2c_is_enabled(HAL_I2C_INTERFACE3, nullptr)) {
            // Unfortunately we cannot return an error code here
            return;
        }
    }
#endif // PLATFORM_ID == PLATFORM_TRACKER

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

void hal_usart_begin(hal_usart_interface_t serial, uint32_t baud) {
    hal_usart_begin_config(serial, baud, SERIAL_8N1, nullptr);
}

void hal_usart_end(hal_usart_interface_t serial) {
    auto usart = getInstance(serial);
    // FIXME: CHECK_XXX?
    if (!usart) {
        return;
    }

    usart->end();
}

int32_t hal_usart_available_data_for_write(hal_usart_interface_t serial) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    return usart->space();
}

int32_t hal_usart_available(hal_usart_interface_t serial) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    return usart->data();
}

void hal_usart_flush(hal_usart_interface_t serial) {
    auto usart = getInstance(serial);
    // FIXME: CHECK_XXX?
    if (!usart) {
        return;
    }

    usart->flush();
}

bool hal_usart_is_enabled(hal_usart_interface_t serial) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), false);

    return usart->isEnabled();
}

ssize_t hal_usart_write_buffer(hal_usart_interface_t serial, const void* buffer, size_t size, size_t elementSize) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    CHECK_TRUE(elementSize == sizeof(uint8_t), SYSTEM_ERROR_INVALID_ARGUMENT);
    usart->pump();
    return usart->write((const uint8_t*)buffer, size);
}

ssize_t hal_usart_read_buffer(hal_usart_interface_t serial, void* buffer, size_t size, size_t elementSize) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    CHECK_TRUE(elementSize == sizeof(uint8_t), SYSTEM_ERROR_INVALID_ARGUMENT);
    return usart->read((uint8_t*)buffer, size);
}

ssize_t hal_usart_peek_buffer(hal_usart_interface_t serial, void* buffer, size_t size, size_t elementSize) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    CHECK_TRUE(elementSize == sizeof(uint8_t), SYSTEM_ERROR_INVALID_ARGUMENT);
    return usart->peek((uint8_t*)buffer, size);
}

int32_t hal_usart_read(hal_usart_interface_t serial) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    uint8_t c;
    CHECK(usart->read(&c, sizeof(c)));
    return c;
}

int32_t hal_usart_peek(hal_usart_interface_t serial) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    uint8_t c;
    CHECK(usart->peek(&c, sizeof(c)));
    return c;
}

uint32_t hal_usart_write_nine_bits(hal_usart_interface_t serial, uint16_t data) {
    return hal_usart_write(serial, data);
}

uint32_t hal_usart_write(hal_usart_interface_t serial, uint8_t data) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    // Blocking!
    while(usart->isEnabled() && usart->space() <= 0) {
        usart->pump();
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
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    auto grp = usart->eventGroup();
    CHECK_TRUE(grp, SYSTEM_ERROR_INVALID_STATE);
    *handle = grp;
    return SYSTEM_ERROR_NONE;
}

int hal_usart_pvt_enable_event(hal_usart_interface_t serial, HAL_USART_Pvt_Events events) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    return usart->enableEvent(events);
}

int hal_usart_pvt_disable_event(hal_usart_interface_t serial, HAL_USART_Pvt_Events events) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    return usart->disableEvent(events);
}

int hal_usart_pvt_wait_event(hal_usart_interface_t serial, uint32_t events, system_tick_t timeout) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    return usart->waitEvent(events, timeout);
}

int hal_usart_sleep(hal_usart_interface_t serial, bool sleep, void* reserved) {
    auto usart = CHECK_TRUE_RETURN(getInstance(serial), SYSTEM_ERROR_NOT_FOUND);
    if (sleep) {
        return usart->suspend();
    } else {
        return usart->restore();
    }
}
