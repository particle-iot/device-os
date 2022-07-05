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

#include "event_group_stream.h"
#include <memory>
#include "spi_hal.h"
#include "ringbuffer.h"

namespace particle {

class Esp32Sdio;

class Esp32SdioStream: public EventGroupBasedStream {
public:
    Esp32SdioStream(hal_spi_interface_t spi, uint32_t clock, hal_pin_t csPin, hal_pin_t intPin);
    ~Esp32SdioStream();

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

    int on(bool on);
    bool on() const;

    EventGroupHandle_t eventGroup() override;

    void txInterruptSupported(bool state);

private:
    int receiveIntoInternal();

private:
    volatile bool enabled_;
    particle::services::RingBuffer<char> rxBuffer_;
    std::unique_ptr<char[]> rxBuf_;
    std::unique_ptr<char[]> txBuf_;
    std::unique_ptr<Esp32Sdio> sdio_;
};

inline void Esp32SdioStream::enabled(bool enabled) {
    enabled_ = enabled;
}

inline bool Esp32SdioStream::enabled() const {
    return enabled_;
}

} // particle
