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

#include "spark_wiring_tether.h"

#if HAL_PLATFORM_PPP_SERVER

namespace particle {

TetherClass Tether;

TetherSerialConfig::TetherSerialConfig()
        : serial_(USARTSerial::from(HAL_PLATFORM_PPP_SERVER_USART)),
          config_(HAL_PLATFORM_PPP_SERVER_USART_FLAGS),
          baudrate_(HAL_PLATFORM_PPP_SERVER_USART_BAUDRATE) {
}

TetherSerialConfig& TetherSerialConfig::serial(USARTSerial& s) {
    serial_ = s;
    return *this;
}

USARTSerial& TetherSerialConfig::serial() const {
    return serial_;
}

TetherSerialConfig& TetherSerialConfig::config(unsigned conf) {
    config_ = conf;
    return *this;
}

unsigned TetherSerialConfig::config() const {
    return config_;
}

TetherSerialConfig& TetherSerialConfig::baudrate(unsigned baud) {
    baudrate_ = baud;
    return *this;
}

unsigned TetherSerialConfig::baudrate() const {
    return baudrate_;
}

int TetherClass::bind(const TetherSerialConfig& config) {
    if_t iface = nullptr;
    if_get_by_index(*this, &iface);
    if (iface) {
        if_req_ppp_server_uart_settings settings = {};
        settings.base.type = IF_REQ_DRIVER_SPECIFIC_PPP_SERVER_UART_SETTINGS;
        settings.serial = config.serial().interface();
        settings.baud = config.baudrate();
        settings.config = config.config();
        return if_request(iface, IF_REQ_DRIVER_SPECIFIC, &settings, sizeof(settings), nullptr);
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

} // spark

#endif // HAL_PLATFORM_PPP_SERVER
