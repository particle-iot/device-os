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

#include "interrupts_hal.h"
#include "gpio_hal.h"
#include "timer_hal.h"
#include "sdio_stream.h"
#include "system_error.h"
#include "service_debug.h"
#include "check.h"


namespace {

const auto SDIO_STREAM_BUFFER_SIZE_RX = 2048;
const auto SDIO_STREAM_BUFFER_SIZE_TX = 2048;

} // anonymous

namespace particle {

SdioStream::SdioStream(HAL_SPI_Interface spi, uint32_t clock, pin_t csPin, pin_t intPin)
        : spi_(spi),
          csPin_(csPin),
          intPin_(intPin),
          clock_(clock),
          enabled_(false),
          context_{},
          rxCount_(0),
          txCount_(0) {

    rxBuffer_.reset(new (std::nothrow) char[SDIO_STREAM_BUFFER_SIZE_RX]);
    txBuffer_.reset(new (std::nothrow) char[SDIO_STREAM_BUFFER_SIZE_TX]);
    SPARK_ASSERT(rxBuffer_);
    SPARK_ASSERT(txBuffer_);
}

SdioStream::~SdioStream() {
    SPARK_ASSERT(hal_sdspi_uninit(spi_, csPin_, intPin_) == SYSTEM_ERROR_NONE);
}

int SdioStream::init() {
    CHECK(hal_sdspi_init(spi_, clock_, csPin_, intPin_));
    enabled_ = true;
    memset(&context_, 0, sizeof(context_));
    flush();
    return SYSTEM_ERROR_NONE;
}

int SdioStream::pollEsp() {
    // Check Wi-Fi interrupt pin immediately
    CHECK(hal_sdspi_wait_intr(0));

    uint32_t intr_raw;
    CHECK(hal_sdspi_get_intr(&intr_raw));

    // Must manually clear interrupt otherwise the interrupt pin will keep low
    CHECK(hal_sdspi_clear_intr(intr_raw));

    size_t sizeRead = 0;
    if (intr_raw & HOST_SLC0_RX_NEW_PACKET_INT) {
        auto r = hal_sdspi_get_packet(&context_, &rxBuffer_.get()[rxCount_], SDIO_STREAM_BUFFER_SIZE_RX - rxCount_, &sizeRead);
        if (r != SYSTEM_ERROR_NONE) {
            LOG(ERROR, "Interrupt is triggered but no data can be read");
        }
        CHECK(r);
    }
    rxCount_ += sizeRead;

    // LOG_PRINTF(INFO, " => RX: size: %d, rxCount_: %d, data: ... ", sizeRead, rxCount_);
    // LOG_DUMP(INFO, rxBuffer_.get(), rxCount_);
    // LOG_PRINTF(INFO, "\r\n");

    return SYSTEM_ERROR_NONE;
}

int SdioStream::read(char* data, size_t size) {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (size == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    pollEsp();

    if (!rxCount_) {
        return 0;
    }

    int sizeRead = rxCount_ > size ? size: rxCount_;
    if (data) {
        memcpy(data, rxBuffer_.get(), sizeRead);
    }

    // Rearrange buffer
    rxCount_ = rxCount_ - sizeRead;
    memcpy(rxBuffer_.get(), &rxBuffer_.get()[sizeRead], rxCount_);
    memset(&rxBuffer_.get()[rxCount_], 0, SDIO_STREAM_BUFFER_SIZE_RX - rxCount_);

    // LOG(INFO, "SdioStream::read, size: %d, data: %s", sizeRead, data);
    // LOG(INFO, "SdioStream::read, rest size: %d, data: %s", rxCount_, rxBuffer_.get());

    return sizeRead;
}

int SdioStream::peek(char* data, size_t size) {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (size == 0) {
        return 0;
    }
    pollEsp();

    int sizeRead = rxCount_ > size ? size: rxCount_;
    if (data) {
        memcpy(data, rxBuffer_.get(), sizeRead);
    }

    return sizeRead;
}

int SdioStream::skip(size_t size) {
    return read(nullptr, size);
}

int SdioStream::write(const char* data, size_t size) {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (size == 0 || size > SDIO_STREAM_BUFFER_SIZE_TX) {
        LOG(ERROR, "SdioStream::write, size overflow!");
        return 0;
    }
    pollEsp();

    // Must copy to RAM, SPI doesn't support to send data in the flash
    txCount_ = size;
    memset(txBuffer_.get(), 0, SDIO_STREAM_BUFFER_SIZE_TX);
    memcpy(txBuffer_.get(), data, size);

    // LOG_PRINTF(INFO, " => TX: size: %d, data: ", txCount_);
    // LOG_DUMP(INFO, txBuffer_.get(), txCount_);
    // LOG_PRINTF(INFO, "\r\n");

    int r = hal_sdspi_send_packet(&context_, txBuffer_.get(), txCount_, 1000*10);
    if (r) {
        return 0;
    }

    return size;
}

int SdioStream::flush() {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    pollEsp();
    rxCount_ = 0;
    return 0;
}

int SdioStream::availForRead() {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    pollEsp();
    return rxCount_;
}

int SdioStream::availForWrite() {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    pollEsp();
    return hal_sdspi_available_for_write(&context_);
}

int SdioStream::waitEvent(unsigned flags, unsigned timeout) {
    if (!enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!flags) {
        return 0;
    }
    if (!(flags & (Stream::READABLE | Stream::WRITABLE))) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    unsigned f = 0;
    const auto t = HAL_Timer_Get_Milli_Seconds();
    for (;;) {
        if (availForRead() > 0) {
            f |= Stream::READABLE;
        }
        if (availForWrite() > 0) {
            f |= Stream::WRITABLE;
        }
        if (f &= flags) {
            break;
        }
        if (timeout > 0 && HAL_Timer_Get_Milli_Seconds() - t >= timeout) {
            return SYSTEM_ERROR_TIMEOUT;
        }
        os_thread_yield();
        if (!enabled_) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
    }
    return f;
}

int SdioStream::on(bool on) {
    return SYSTEM_ERROR_NONE;
}

bool SdioStream::on() const {
    return true;
}

EventGroupHandle_t SdioStream::eventGroup() {
    return nullptr;
}

} // particle
