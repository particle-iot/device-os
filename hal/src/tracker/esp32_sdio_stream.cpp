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

// FIXME: pinmap definitions caused problems when nrf52840.h is included, so as a simple workaround include it here first
#include <nrf52840.h>
#include "esp32_sdio_stream.h"
#include "system_error.h"
#include "check.h"
#include "sdspi/esp32_sdio.h"
#include "service_debug.h"
#include "timer_hal.h"

#undef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ALL

namespace {

const size_t SDIO_STREAM_BUFFER_SIZE_RX = 1024;
const size_t SDIO_STREAM_BUFFER_SIZE_TX = 512;

} // anonymous

namespace particle {

Esp32SdioStream::Esp32SdioStream(hal_spi_interface_t spi, uint32_t clock, hal_pin_t csPin, hal_pin_t intPin)
        : enabled_(false) {

    rxBuf_.reset(new (std::nothrow) char[SDIO_STREAM_BUFFER_SIZE_RX]);
    txBuf_.reset(new (std::nothrow) char[SDIO_STREAM_BUFFER_SIZE_TX]);
    SPARK_ASSERT(rxBuf_);
    SPARK_ASSERT(txBuf_);

    rxBuffer_.init(rxBuf_.get(), SDIO_STREAM_BUFFER_SIZE_RX);

    hal_spi_info_t conf = {
        .version = HAL_SPI_INFO_VERSION,
        .system_clock = 0,
        .default_settings = 0,
        .enabled = true,
        .mode = SPI_MODE_MASTER,
        .clock = clock,
        .bit_order = MSBFIRST,
        .data_mode = SPI_MODE0,
        .ss_pin = PIN_INVALID
    };
    sdio_ = std::make_unique<Esp32Sdio>(spi, &conf, csPin, intPin);
    SPARK_ASSERT(sdio_);
}

Esp32SdioStream::~Esp32SdioStream() {
}

int Esp32SdioStream::init() {
    CHECK(sdio_->init());
    enabled_ = true;
    flush();
    return SYSTEM_ERROR_NONE;
}

int Esp32SdioStream::read(char* data, size_t size) {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (size == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    size_t canRead = CHECK(rxBuffer_.data());
    if (canRead == 0) {
        canRead = CHECK(receiveIntoInternal());
    }
    size_t willRead = std::min(canRead, size);
    auto r = rxBuffer_.get(data, willRead);
    return r;
}

int Esp32SdioStream::peek(char* data, size_t size) {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (size == 0) {
        return 0;
    }
    size_t canPeek = CHECK(rxBuffer_.data());
    size_t willPeek = std::min(canPeek, size);
    return rxBuffer_.peek(data, willPeek);
}

int Esp32SdioStream::skip(size_t size) {
    return read(nullptr, size);
}

int Esp32SdioStream::write(const char* data, size_t size) {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (size == 0) {
        return 0;
    }

    // FIXME: this should be handled in SPI HAL (https://github.com/particle-iot/device-os/pull/2196)
    size_t pos = 0;
    while (size) {
        size_t toCopy = std::min(SDIO_STREAM_BUFFER_SIZE_TX, size);
        memcpy(txBuf_.get(), data + pos, toCopy);

        size_t written = CHECK(sdio_->write((const uint8_t*)txBuf_.get(), toCopy));
        size -= written;
        pos += written;
        if (written < toCopy) {
            break;
        }
    }
    return pos;
}

int Esp32SdioStream::flush() {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    return 0;
}

int Esp32SdioStream::availForRead() {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    return rxBuffer_.data();
}

int Esp32SdioStream::availForWrite() {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    return sdio_->txBufferSize();
}

int Esp32SdioStream::waitEvent(unsigned flags, unsigned timeout) {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!flags) {
        return 0;
    }

    if (flags & Stream::READABLE) {
        if (availForRead() > 0) {
            xEventGroupSetBits(eventGroup(), Stream::READABLE);
        }
    }

    // NOTE: non-Stream events may be passed here
    auto r = CHECK(sdio_->waitEvent(flags, timeout));
    if (r & Stream::READABLE) {
        receiveIntoInternal();
    }
    return r;
}

int Esp32SdioStream::receiveIntoInternal() {
    rxBuffer_.acquireBegin();
    const size_t acquirable = rxBuffer_.acquirable();
    const size_t acquirableWrapped = rxBuffer_.acquirableWrapped();
    size_t rxSize = std::max(acquirable, acquirableWrapped);
    // IMPORTANT: esp32 sdspi driver will align reads to 4 bytes and may
    // write data past the provided size. Align the buffer size to the left
    // to avoid that.
    rxSize -= (rxSize % 4);
    if (rxSize >= sizeof(uint32_t)) {
        auto ptr = rxBuffer_.acquire(rxSize);
        auto toCommit = std::max(sdio_->read((uint8_t*)ptr, rxSize), 0);
        rxBuffer_.acquireCommit(toCommit, rxSize - toCommit);
        return toCommit;
    }

    return 0;
}

int Esp32SdioStream::on(bool on) {
    return sdio_->on(on);
}

bool Esp32SdioStream::on() const {
    return true;
}

EventGroupHandle_t Esp32SdioStream::eventGroup() {
    return sdio_->eventGroup();
}

void Esp32SdioStream::txInterruptSupported(bool state) {
    sdio_->txInterruptSupported(state);
}

} // particle
