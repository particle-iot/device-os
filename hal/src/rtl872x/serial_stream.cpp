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

#include "serial_stream.h"

#include "concurrent_hal.h"
#include "timer_hal.h"
#include "service_debug.h"
#include "system_error.h"

namespace {

const auto SERIAL_STREAM_BUFFER_SIZE_RX = 2048;
const auto SERIAL_STREAM_BUFFER_SIZE_TX = 2048;

} // anonymous

namespace particle {

SerialStream::SerialStream(hal_usart_interface_t serial, uint32_t baudrate, uint32_t config,
        size_t rxBufferSize, size_t txBufferSize)
        : serial_(serial),
          config_(config),
          baudrate_(baudrate),
          enabled_(true),
          phyOn_(false) {

    if (!rxBufferSize) {
        rxBufferSize = SERIAL_STREAM_BUFFER_SIZE_RX;
    }
    if (!txBufferSize) {
        txBufferSize = SERIAL_STREAM_BUFFER_SIZE_TX;
    }

    rxBuffer_.reset(new (std::nothrow) char[rxBufferSize]);
    txBuffer_.reset(new (std::nothrow) char[txBufferSize]);
    SPARK_ASSERT(rxBuffer_);
    SPARK_ASSERT(txBuffer_);

    hal_usart_buffer_config_t c = {};
    c.size = sizeof(c);
    c.rx_buffer = (uint8_t*)rxBuffer_.get();
    c.tx_buffer = (uint8_t*)txBuffer_.get();
    c.rx_buffer_size = rxBufferSize;
    c.tx_buffer_size = txBufferSize;
    hal_usart_init_ex(serial_, &c, nullptr);
    hal_usart_begin_config(serial_, baudrate, config, 0);
    phyOn_ = true;
}

SerialStream::~SerialStream() {
    hal_usart_end(serial_);
}

int SerialStream::read(char* data, size_t size) {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (size == 0) {
        return 0;
    }
    auto r = hal_usart_read_buffer(serial_, data, size, sizeof(char));
    if (r == SYSTEM_ERROR_NO_MEMORY) {
        return 0;
    }
    return r;
}

int SerialStream::peek(char* data, size_t size) {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (size == 0) {
        return 0;
    }
    auto r = hal_usart_peek_buffer(serial_, data, size, sizeof(char));
    if (r == SYSTEM_ERROR_NO_MEMORY) {
        return 0;
    }
    return r;
}

int SerialStream::skip(size_t size) {
    return read(nullptr, size);
}

int SerialStream::write(const char* data, size_t size) {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (size == 0) {
        return 0;
    }
    auto r = hal_usart_write_buffer(serial_, data, size, sizeof(char));
    if (r == SYSTEM_ERROR_NO_MEMORY) {
        return 0;
    }
    return r;
}

int SerialStream::flush() {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    hal_usart_flush(serial_);
    return 0;
}

int SerialStream::availForRead() {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    return hal_usart_available(serial_);
}

int SerialStream::availForWrite() {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    return hal_usart_available_data_for_write(serial_);
}

int SerialStream::waitEvent(unsigned flags, unsigned timeout) {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!flags) {
        return 0;
    }

    // NOTE: non-Stream events may be passed here

    return hal_usart_pvt_wait_event(serial_, flags, timeout);
}

int SerialStream::setBaudRate(unsigned int baudrate) {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    hal_usart_end(serial_);
    phyOn_ = false;
    hal_usart_begin_config(serial_, baudrate, config_, 0);
    baudrate_ = baudrate;
    phyOn_ = true;
    return 0;
}

int SerialStream::setConfig(uint32_t config, unsigned int baudrate /* optional */) {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    hal_usart_end(serial_);
    phyOn_ = false;
    if (baudrate != 0) {
        baudrate_ = baudrate;
    }
    config_ = config;
    hal_usart_begin_config(serial_, baudrate_, config_, 0);
    phyOn_ = true;
    return 0;
}

int SerialStream::on(bool on) {
    if (on) {
        CHECK_FALSE(phyOn_, SYSTEM_ERROR_NONE);
        hal_usart_begin_config(serial_, baudrate_, config_, 0);
        phyOn_ = true;
    } else {
        CHECK_TRUE(phyOn_, SYSTEM_ERROR_NONE);
        hal_usart_end(serial_);
        phyOn_ = false;
    }
    return SYSTEM_ERROR_NONE;
}

EventGroupHandle_t SerialStream::eventGroup() {
    EventGroupHandle_t ev = nullptr;
    hal_usart_pvt_get_event_group_handle(serial_, &ev);
    return ev;
}

} // particle
