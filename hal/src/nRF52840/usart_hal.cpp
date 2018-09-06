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
#include "usart_hal.h"
#include <nrf_uarte.h>
#include "ringbuffer.h"
#include "pinmap_impl.h"
#include "check_nrf.h"
#include "gpio_hal.h"
#include <nrfx_prs.h>
#include <nrf_gpio.h>
#include <atomic>
#include <algorithm>
#include "hal_irq_flag.h"

namespace {

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
const size_t RESERVED_RX_SIZE = 5;

class Usart {
public:
    Usart(NRF_UARTE_Type* instance, void (*interruptHandler)(void),
            pin_t tx, pin_t rx, pin_t cts, pin_t rts)
            : uarte_(instance),
              interruptHandler_(interruptHandler),
              txPin_(tx),
              rxPin_(rx),
              ctsPin_(cts),
              rtsPin_(rts),
              transmitting_(false),
              receiving_(0) {
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
        rxBuffer_.reset();
        txBuffer_.reset();
        configured_ = false;

        return 0;
    }

    int begin(const Config& conf) {
        CHECK_TRUE(isConfigured(), SYSTEM_ERROR_INVALID_STATE);
        CHECK_TRUE(validateConfig(conf.config), SYSTEM_ERROR_INVALID_ARGUMENT);
        auto nrfBaudRate = CHECK_RETURN(getNrfBaudrate(conf.baudRate), SYSTEM_ERROR_INVALID_ARGUMENT);

        if (isEnabled()) {
            end();
        }

        CHECK_NRF_RETURN(nrfx_prs_acquire(uarte_, interruptHandler_), SYSTEM_ERROR_INTERNAL);

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
        nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_TXDDY);
        nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_TXSTOPPED);
        nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ENDTX);

        nrf_uarte_int_enable(uarte_, NRF_UARTE_INT_ERROR_MASK |
                                     NRF_UARTE_INT_RXSTARTED_MASK |
                                     NRF_UARTE_INT_RXDRDY_MASK |
                                     NRF_UARTE_INT_RXTO_MASK |
                                     NRF_UARTE_INT_ENDRX_MASK |
                                     NRF_UARTE_INT_TXSTARTED_MASK |
                                     NRF_UARTE_INT_TXDRDY_MASK |
                                     NRF_UARTE_INT_TXSTOPPED_MASK |
                                     NRF_UARTE_INT_ENDTX_MASK);

        NRFX_IRQ_PRIORITY_SET(nrfx_get_irq_number((void *)uarte_), APP_IRQ_PRIORITY_LOWEST);
        NRFX_IRQ_ENABLE(nrfx_get_irq_number((void *)uarte_));

        nrf_uarte_enable(uarte_);

        config_ = conf;
        enabled_ = true;

        startReceiver();

        return 0;
    }

    int end() {
        CHECK_TRUE(enabled_, SYSTEM_ERROR_INVALID_STATE);

        nrf_uarte_disable(uarte_);
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

        enabled_ = false;
        config_ = {};
        transmitting_ = false;
        receiving_ = 0;

        return 0;
    }

    ssize_t data() {
        AtomicSection lk;
        return rxBuffer_.data();
    }

    ssize_t space() {
        AtomicSection lk;
        return txBuffer_.space();
    }

    ssize_t read(uint8_t* buffer, size_t size) {
        AtomicSection lk;
        ssize_t r = CHECK(rxBuffer_.get(buffer, size));
        if (!receiving_) {
            startReceiver();
        }
        return r;
    }

    ssize_t peek(uint8_t* buffer, size_t size) {
        AtomicSection lk;
        return rxBuffer_.peek(buffer, size);
    }

    ssize_t write(const uint8_t* buffer, size_t size) {
        AtomicSection lk;
        ssize_t r = CHECK(txBuffer_.put(buffer, size));
        if (!transmitting_) {
            // Start transmission
            startTransmission();
        }
        return r;
    }

    ssize_t flush() {
        while (true) {
            while (transmitting_) {
                // FIXME: busy loop
            }
            {
                AtomicSection lk;
                if (txBuffer_.empty()) {
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

    void interruptHandler() {
        // nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ERROR);

        // nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_RXSTARTED);
        // nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_RXDRDY);
        // nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_RXTO);
        // nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ENDRX);

        // nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_TXSTARTED);
        // nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_TXDDY);
        // nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_TXSTOPPED);
        // nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ENDTX);

        if (nrf_uarte_event_check(uarte_, NRF_UARTE_EVENT_ERROR)) {
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ERROR);
            const auto e = nrf_uarte_errorsrc_get_and_clear(uarte_);
            LOG_DEBUG(TRACE, "event NRF_UARTE_EVENT_ERROR %u", e);
            (void)e;
        }
        if (nrf_uarte_event_check(uarte_, NRF_UARTE_EVENT_RXSTARTED)) {
            // LOG_DEBUG(TRACE, "event NRF_UARTE_EVENT_RXSTARTED");
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_RXSTARTED);
            startReceiver();
        }
        if (nrf_uarte_event_check(uarte_, NRF_UARTE_EVENT_RXDRDY)) {
            // LOG_DEBUG(TRACE, "event NRF_UARTE_EVENT_RXDRDY");
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_RXDRDY);
            rxBuffer_.acquireCommit(1);
        }
        if (nrf_uarte_event_check(uarte_, NRF_UARTE_EVENT_RXTO)) {
            LOG_DEBUG(TRACE, "event NRF_UARTE_EVENT_RXTO");
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_RXTO);
            // Flush
            startReceiver(true);
        }
        if (nrf_uarte_event_check(uarte_, NRF_UARTE_EVENT_ENDRX)) {
            LOG_DEBUG(TRACE, "event NRF_UARTE_EVENT_ENDRX");
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ENDRX);
            --receiving_;
            startReceiver();
        }
        if (nrf_uarte_event_check(uarte_, NRF_UARTE_EVENT_TXSTARTED)) {
            // LOG_DEBUG(TRACE, "event NRF_UARTE_EVENT_TXSTARTED");
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_TXSTARTED);
        }
        if (nrf_uarte_event_check(uarte_, NRF_UARTE_EVENT_TXDDY)) {
            // LOG_DEBUG(TRACE, "event NRF_UARTE_EVENT_TXDDY");
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_TXDDY);
            txBuffer_.consumeCommit(1);
        }
        if (nrf_uarte_event_check(uarte_, NRF_UARTE_EVENT_TXSTOPPED)) {
            // LOG_DEBUG(TRACE, "event NRF_UARTE_EVENT_TXSTOPPED");
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_TXSTOPPED);
        }
        if (nrf_uarte_event_check(uarte_, NRF_UARTE_EVENT_ENDTX)) {
            // LOG_DEBUG(TRACE, "event NRF_UARTE_EVENT_ENDTX");
            nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ENDTX);
            transmitting_ = false;
            startTransmission();
        }
    }

private:
    void startTransmission() {
        const size_t consumable = txBuffer_.consumable();
        if (!transmitting_ && consumable > 0) {
            auto ptr = txBuffer_.consume(consumable);
            SPARK_ASSERT(ptr);
            // nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ENDTX);
            // nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_TXSTOPPED);
            nrf_uarte_tx_buffer_set(uarte_, ptr, consumable);
            nrf_uarte_task_trigger(uarte_, NRF_UARTE_TASK_STARTTX);
        }
    }

    void startReceiver(bool flush = false) {
        LOG_DEBUG(TRACE, "startReceiver: %d", (int)receiving_);
        if (receiving_ >= MAX_SCHEDULED_RECEIVALS) {
            return;
        }

        const size_t acquirable = rxBuffer_.acquirable();
        const size_t acquirableWrapped = rxBuffer_.acquirableWrapped();
        size_t rxSize = std::max(acquirable, acquirableWrapped);

        LOG_DEBUG(TRACE, "acquirable %d %d %d", acquirable, acquirableWrapped, rxSize);

        if (!flush) {
            // We should always reserve 5 bytes in the rx buffer
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
            SPARK_ASSERT(rxSize == RESERVED_RX_SIZE);
        }

        LOG_DEBUG(TRACE, "rxSize %d", rxSize);

        if (rxSize > 0) {
            ++receiving_;
            auto ptr = rxBuffer_.acquire(rxSize);
            SPARK_ASSERT(ptr);
            // nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_ENDRX);
            // nrf_uarte_event_clear(uarte_, NRF_UARTE_EVENT_RXTO);
            nrf_uarte_rx_buffer_set(uarte_, ptr, rxSize);
            if (!flush) {
                LOG_DEBUG(TRACE, "startrx");
                nrf_uarte_task_trigger(uarte_, NRF_UARTE_TASK_STARTRX);
            } else {
                LOG_DEBUG(TRACE, "flushrx");
                nrf_uarte_task_trigger(uarte_, NRF_UARTE_TASK_FLUSHRX);
            }
        }
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

    pin_t txPin_;
    pin_t rxPin_;
    pin_t ctsPin_;
    pin_t rtsPin_;

    bool configured_ = false;
    bool enabled_ = false;

    std::atomic_bool transmitting_;
    std::atomic<uint8_t> receiving_;

    Config config_ = {};

    particle::services::RingBuffer<uint8_t> txBuffer_;
    particle::services::RingBuffer<uint8_t> rxBuffer_;
};

constexpr const Usart::BaudrateMap Usart::baudrateMap_[];


static Usart s_usartMap[] = {
    {NRF_UARTE0, uarte0InterruptHandler, TX, RX, CTS, RTS},
    {NRF_UARTE1, uarte1InterruptHandler, TX1, RX1, CTS1, RTS1}
};

Usart* getInstance(HAL_USART_Serial serial) {
    CHECK_TRUE(serial < sizeof(s_usartMap) / sizeof(s_usartMap[0]), nullptr);

    return &s_usartMap[serial];
}

void uarte0InterruptHandler(void) {
    getInstance(HAL_USART_SERIAL1)->interruptHandler();
}

void uarte1InterruptHandler(void) {
    getInstance(HAL_USART_SERIAL2)->interruptHandler();
}

} // anonymous

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

void HAL_USART_Send_Break(HAL_USART_Serial serial, void* reserved) {
    // not support
    return;
}

uint8_t HAL_USART_Break_Detected(HAL_USART_Serial serial) {
    // not support
    return false;
}
