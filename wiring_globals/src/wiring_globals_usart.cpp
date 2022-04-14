/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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


#include "spark_wiring_usartserial.h"
#include "usart_hal.h"
#include <new>

#ifndef SPARK_WIRING_NO_USART_SERIAL

namespace {

hal_usart_buffer_config_t defaultUsartConfig() {
#if !HAL_PLATFORM_USART_9BIT_SUPPORTED
    const size_t bufferSize = SERIAL_BUFFER_SIZE;
#else
    const size_t bufferSize = SERIAL_BUFFER_SIZE * sizeof(uint16_t);
#endif // HAL_PLATFORM_USART_9BIT_SUPPORTED

    hal_usart_buffer_config_t config = {
        .size = sizeof(hal_usart_buffer_config_t),
        .rx_buffer = new (std::nothrow) uint8_t[bufferSize],
        .rx_buffer_size = bufferSize,
        .tx_buffer = new (std::nothrow) uint8_t[bufferSize],
        .tx_buffer_size = bufferSize
    };

    return config;
}

} // anonymous

hal_usart_buffer_config_t __attribute__((weak)) acquireSerial1Buffer()
{
    return defaultUsartConfig();
}

#if Wiring_Serial2
hal_usart_buffer_config_t __attribute__((weak)) acquireSerial2Buffer()
{
    return defaultUsartConfig();
}
#endif

#if Wiring_Serial3
hal_usart_buffer_config_t __attribute__((weak)) acquireSerial3Buffer()
{
    return defaultUsartConfig();
}
#endif

#if Wiring_Serial4
hal_usart_buffer_config_t __attribute__((weak)) acquireSerial4Buffer()
{
    return defaultUsartConfig();
}
#endif

#if Wiring_Serial5
hal_usart_buffer_config_t __attribute__((weak)) acquireSerial5Buffer()
{
    return defaultUsartConfig();
}
#endif

USARTSerial& __fetch_global_Serial1()
{
    static USARTSerial serial1(HAL_USART_SERIAL1, acquireSerial1Buffer());
    return serial1;
}

// optional SerialX is instantiated from libraries/SerialX/SerialX.h
#endif // SPARK_WIRING_NO_USART_SERIAL
