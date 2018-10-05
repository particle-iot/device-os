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

#include "system_error.h"

namespace particle {

SerialStream::SerialStream(HAL_USART_Serial serial, uint32_t baudrate, uint32_t config) :
        rxBuffer_(),
        txBuffer_(),
        serial_(serial) {
    HAL_USART_Init(serial_, &rxBuffer_, &txBuffer_);
    HAL_USART_BeginConfig(serial_, baudrate, config, 0);
}

SerialStream::~SerialStream() {
    HAL_USART_End(serial_);
}

int SerialStream::read(char* data, size_t size) {
    size_t read = 0;
    while (read < size) {
        int c = HAL_USART_Read_Data(serial_);
        if (c < 0) {
            break;
        }
        data[read++] = (char)c;
    }
    return read;
}

int SerialStream::peek(char* data, size_t size) {
    if (size > 1) {
        return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
    }
    if (size == 0) {
        return 0;
    }
    const int c = HAL_USART_Peek_Data(serial_);
    if (c < 0) {
        return 0;
    }
    *data = c;
    return size;
}

int SerialStream::skip(size_t size) {
    size_t n = 0;
    while (n < size) {
        const int c = HAL_USART_Read_Data(serial_);
        if (c < 0) {
            break;
        }
        ++n;
    }
    return n;
}

int SerialStream::write(const char* data, size_t size) {
    size_t written = 0;
    while (written < size) {
        auto w = HAL_USART_Write_NineBitData(serial_, data[written]);
        if (w == sizeof(data[written])) {
            ++written;
        }
    }
    return written;
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
