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

SerialStream::SerialStream(HAL_USART_Serial serial, uint32_t baudrate, uint32_t config,
        size_t rxBufferSize, size_t txBufferSize) :
        serial_(serial) {

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

    HAL_USART_Buffer_Config c = {};
    c.size = sizeof(c);
    c.rx_buffer = (uint8_t*)rxBuffer_.get();
    c.tx_buffer = (uint8_t*)txBuffer_.get();
    c.rx_buffer_size = rxBufferSize;
    c.tx_buffer_size = txBufferSize;
    HAL_USART_Init_Ex(serial_, &c, nullptr);
    HAL_USART_BeginConfig(serial_, baudrate, config, 0);
}

SerialStream::~SerialStream() {
    HAL_USART_End(serial_);
}

int SerialStream::read(char* data, size_t size) {
    if (size == 0) {
        return 0;
    }
    return HAL_USART_Read(serial_, data, size, sizeof(char));
}

int SerialStream::peek(char* data, size_t size) {
    if (size == 0) {
        return 0;
    }
    return HAL_USART_Peek(serial_, data, size, sizeof(char));
}

int SerialStream::skip(size_t size) {
    return HAL_USART_Read(serial_, nullptr, size, sizeof(char));
}

int SerialStream::write(const char* data, size_t size) {
    auto r = HAL_USART_Write(serial_, data, size, sizeof(char));
    if (r == SYSTEM_ERROR_NO_MEMORY) {
        return 0;
    }
    return r;
}

int SerialStream::flush() {
    HAL_USART_Flush_Data(serial_);
    return 0;
}

int SerialStream::availForRead() {
    return HAL_USART_Available_Data(serial_);
}

int SerialStream::availForWrite() {
    return HAL_USART_Available_Data_For_Write(serial_);
}

int SerialStream::waitEvent(unsigned flags, unsigned timeout) {
    if (!flags) {
        return 0;
    }
    if (!(flags & (Stream::READABLE | Stream::WRITABLE))) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    unsigned f = 0;
    const auto t = HAL_Timer_Get_Milli_Seconds();
    for (;;) {
        if (HAL_USART_Available_Data(serial_) > 0) {
            f |= Stream::READABLE;
        }
        if (HAL_USART_Available_Data_For_Write(serial_) > 0) {
            f |= Stream::WRITABLE;
        }
        if (f &= flags) {
            break;
        }
        if (timeout > 0 && HAL_Timer_Get_Milli_Seconds() - t >= timeout) {
            return SYSTEM_ERROR_TIMEOUT;
        }
        os_thread_yield();
    }
    return f;
}

} // particle
