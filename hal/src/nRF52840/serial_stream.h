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

#pragma once

#include "usart_hal.h"
#include "stream.h"
#include <memory>

namespace particle {

class SerialStream: public Stream {
public:
    SerialStream(HAL_USART_Serial serial, uint32_t baudrate, uint32_t config,
            size_t rxBufferSize = 0, size_t txBufferSize = 0);
    ~SerialStream();

    int read(char* data, size_t size) override;
    int peek(char* data, size_t size) override;
    int skip(size_t size) override;
    int write(const char* data, size_t size) override;
    int flush() override;
    int availForRead() override;
    int availForWrite() override;
    int waitEvent(unsigned flags, unsigned timeout) override;

    int setBaudRate(unsigned int baudrate);

    void enabled(bool enabled);
    bool enabled() const;

private:
    HAL_USART_Serial serial_;
    std::unique_ptr<char[]> rxBuffer_;
    std::unique_ptr<char[]> txBuffer_;
    uint32_t config_;
    volatile bool enabled_;
};

inline void SerialStream::enabled(bool enabled) {
    enabled_ = enabled;
}

inline bool SerialStream::enabled() const {
    return enabled_;
}

} // particle
