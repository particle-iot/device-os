/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include "usb_hal.h"
#include "usb_hal_private.h"
#include "event_group_stream.h"
#include "check.h"
#include <memory>

namespace particle {

class SerialUSBStream: public EventGroupBasedStream {
public:
    SerialUSBStream(HAL_USB_USART_Serial serial, uint32_t baudrate, size_t rxBufferSize = 0, size_t txBufferSize = 0);
    ~SerialUSBStream();

    int read(char* data, size_t size) override;
    int peek(char* data, size_t size) override;
    int skip(size_t size) override;
    int write(const char* data, size_t size) override;
    int flush() override;
    int availForRead() override;
    int availForWrite() override;
    int waitEvent(unsigned flags, unsigned timeout) override;

    int setBaudRate(unsigned int baudrate);
    int setConfig(uint32_t config, unsigned int baudrate = 0);

    uint32_t config() const;
    unsigned int baudrate() const;

    void enabled(bool enabled);
    bool enabled() const;

    int on(bool on);
    bool on() const;

    EventGroupHandle_t eventGroup() override;

private:
    HAL_USB_USART_Serial serial_;
    std::unique_ptr<char[]> rxBuffer_;
    std::unique_ptr<char[]> txBuffer_;
    uint32_t config_;
    uint32_t baudrate_;
    volatile bool enabled_;
    volatile bool phyOn_;
};

inline void SerialUSBStream::enabled(bool enabled) {
    enabled_ = enabled;
}

inline bool SerialUSBStream::enabled() const {
    return enabled_;
}

inline bool SerialUSBStream::on() const {
    return phyOn_;
}

inline uint32_t SerialUSBStream::config() const {
    return config_;
}

inline unsigned int SerialUSBStream::baudrate() const {
    return baudrate_;
}

} // particle
