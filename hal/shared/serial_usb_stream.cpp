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

#include "hal_platform.h"

#if HAL_PLATFORM_PPP_SERVER 
#if HAL_PLATFORM_PPP_SERVER_USB

#include "serial_usb_stream.h"

#include "concurrent_hal.h"
#include "timer_hal.h"
#include "service_debug.h"
#include "system_error.h"
#include <algorithm>
#if HAL_PLATFORM_NRF52840
#include "usb_hal_cdc.h"
#endif
#if HAL_PLATFORM_RTL872X
#include "usb_hal.h"
#endif

namespace {

const auto SERIAL_STREAM_BUFFER_SIZE_RX = 2048;
const auto SERIAL_STREAM_BUFFER_SIZE_TX = 2048;

} // anonymous

namespace particle {
SerialUSBStream::SerialUSBStream(HAL_USB_USART_Serial serial, uint32_t baudrate, size_t rxBufferSize, size_t txBufferSize)
        : serial_(serial),
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

    HAL_USB_USART_Config c = {};
    c.size = sizeof(c);
    c.rx_buffer = (uint8_t*)rxBuffer_.get();
    c.tx_buffer = (uint8_t*)txBuffer_.get();
    c.rx_buffer_size = rxBufferSize;
    c.tx_buffer_size = txBufferSize;

    HAL_USB_USART_Init(serial_, &c);
    HAL_USB_USART_Begin(serial_, baudrate_, nullptr);
    phyOn_ = true;
}

SerialUSBStream::~SerialUSBStream() {
    HAL_USB_USART_End(serial_);
}

int SerialUSBStream::read(char* data, size_t size) {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (size == 0) {
        return 0;
    }

    return hal_usb_cdc_pvt_recv_data(data, size);
}

int SerialUSBStream::peek(char* data, size_t size) {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (size == 0) {
        return 0;
    }
    return HAL_USB_USART_Peek_Buffer(serial_, data, size);
}

int SerialUSBStream::skip(size_t size) {
    return read(nullptr, size);
}

int SerialUSBStream::write(const char* data, size_t size) {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (size == 0) {
        return 0;
    }

    int32_t available = HAL_USB_USART_Available_Data_For_Write(serial_);
    if (available <= 0) {
        return 0;
    }

    size_t writeSize = std::min((size_t)available, size);
    auto r = hal_usb_cdc_pvt_send_data(data, writeSize);
    if (r < 0) {
        return r;
    }
    return r;
}

int SerialUSBStream::flush() {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    HAL_USB_USART_Flush_Data(serial_);
    return 0;
}

int SerialUSBStream::availForRead() {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    return HAL_USB_USART_Available_Data(serial_);
}

int SerialUSBStream::availForWrite() {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    return HAL_USB_USART_Available_Data_For_Write(serial_);
}

int SerialUSBStream::waitEvent(unsigned flags, unsigned timeout) {
    if (!phyOn_ || !enabled_) {
      return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!flags) {
      return 0;
    }

    return hal_usb_cdc_pvt_wait_event(flags, timeout);
}

int SerialUSBStream::setBaudRate(unsigned int baudrate) {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    HAL_USB_USART_End(serial_);
    phyOn_ = false;
    HAL_USB_USART_Begin(serial_, baudrate_, nullptr);
    baudrate_ = baudrate;
    phyOn_ = true;
    return 0;
}

// CTS/RTS/Baudrate Configs are no ops
int SerialUSBStream::setConfig(uint32_t config, unsigned int baudrate /* optional */) {
    return 0;
}

int SerialUSBStream::on(bool on) {
    if (on) {
        CHECK_FALSE(phyOn_, SYSTEM_ERROR_NONE);
        HAL_USB_USART_Begin(serial_, baudrate_, nullptr);
        phyOn_ = true;
    } else {
        CHECK_TRUE(phyOn_, SYSTEM_ERROR_NONE);
        HAL_USB_USART_End(serial_);
        phyOn_ = false;
    }
    return SYSTEM_ERROR_NONE;
}

EventGroupHandle_t SerialUSBStream::eventGroup() {
    EventGroupHandle_t ev = nullptr;
    hal_usb_cdc_pvt_get_event_group_handle(&ev);
    return ev;
}

} // particle
#endif // HAL_PLATFORM_PPP_SERVER
#endif // HAL_PLATFORM_PPP_SERVER_USB