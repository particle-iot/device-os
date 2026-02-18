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

#include "serial_usb_stream.h"

#include "concurrent_hal.h"
#include "timer_hal.h"
#include "service_debug.h"
#include "system_error.h"
#include "delay_hal.h"
#include "usb_hal_cdc.h"

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
    // TODO:
}

int SerialUSBStream::read(char* data, size_t size) {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (size == 0) {
        return 0;
    }
    
    for (size_t i = 0; i < size; i++) {
        auto receivedByte = HAL_USB_USART_Receive_Data(serial_, 0);
        if (receivedByte < 0) {
            return i;    
        }
        // TODO: HANDLE NULL PTR CASE LIKE FOR SKIP
        *data++ = receivedByte;
    }

    return size;
}

// TODO: not used by AT server / pppserver
int SerialUSBStream::peek(char* data, size_t size) {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (size == 0) {
        return 0;
    }
    auto r = 0;
    if (r == SYSTEM_ERROR_NO_MEMORY) {
        return 0;
    }
    return r;
}

// TODO: not used by AT server / pppserver
int SerialUSBStream::skip(size_t size) {
    return read(nullptr, size);
}

// TODO: ********************************************************************************
int SerialUSBStream::write(const char* data, size_t size) {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (size == 0) {
        return 0;
    }
    
    for (size_t i = 0; i < size; i++) {
        auto r = HAL_USB_USART_Send_Data(serial_, data[i]);
        if (r < 0) {
            return i;
        }
    }
    return size;
}

int SerialUSBStream::flush() {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // TODO:
    return 0;
}

int SerialUSBStream::availForRead() {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    return HAL_USB_USART_Available_Data(serial_);
}

// TODO: TEST least important for now, only used for gsm muxer implementation
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

// TODO
int SerialUSBStream::setBaudRate(unsigned int baudrate) {
    if (!phyOn_ || !enabled_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // hal_usart_end(serial_);
    // phyOn_ = false;
    // hal_usart_begin_config(serial_, baudrate, config_, 0);
    // baudrate_ = baudrate;
    // phyOn_ = true;
    return 0;
}

// TODO: Do we need this
// int SerialUSBStream::setConfig(uint32_t config, unsigned int baudrate /* optional */) {
//     if (!phyOn_ || !enabled_) {
//         return SYSTEM_ERROR_INVALID_STATE;
//     }
//     hal_usart_end(serial_);
//     phyOn_ = false;
//     if (baudrate != 0) {
//         baudrate_ = baudrate;
//     }
//     config_ = config;
//     hal_usart_begin_config(serial_, baudrate_, config_, 0);
//     phyOn_ = true;
//     return 0;
// }

// TODO: is this called? 
int SerialUSBStream::on(bool on) {
    if (on) {
        CHECK_FALSE(phyOn_, SYSTEM_ERROR_NONE);
        // hal_usart_begin_config(serial_, baudrate_, config_, 0);
        phyOn_ = true;
    } else {
        CHECK_TRUE(phyOn_, SYSTEM_ERROR_NONE);
        // hal_usart_end(serial_);
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
