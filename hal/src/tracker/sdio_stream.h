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

#pragma once

#include "stream.h"
#include "sdspi_hal.h"
#include <memory>

namespace particle {

class SdioStream: public Stream {
public:
    SdioStream(HAL_SPI_Interface spi, uint32_t clockDivider, pin_t csPin, pin_t intPin);
    ~SdioStream();

    int read(char* data, size_t size) override;
    int peek(char* data, size_t size) override;
    int skip(size_t size) override;
    int write(const char* data, size_t size) override;
    int flush() override;
    int availForRead() override;
    int availForWrite() override;
    int waitEvent(unsigned flags, unsigned timeout) override;

    int init();
    void enabled(bool enabled);
    bool enabled() const;

private:
    int pollEsp();

private:
    HAL_SPI_Interface spi_;
    pin_t csPin_;
    pin_t intPin_;
    uint32_t clock_;
    volatile bool enabled_;
    spi_context_t context_;
    std::unique_ptr<char[]> rxBuffer_;
    std::unique_ptr<char[]> txBuffer_;
    size_t rxCount_;
    size_t txCount_;
};

inline void SdioStream::enabled(bool enabled) {
    enabled_ = enabled;
}

inline bool SdioStream::enabled() const {
    return enabled_;
}

} // particle
