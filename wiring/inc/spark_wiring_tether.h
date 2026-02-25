/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

#include "spark_wiring_platform.h"

#if HAL_PLATFORM_PPP_SERVER

#include "spark_wiring_network.h"
#include "system_network.h"
#include "spark_wiring_usartserial.h"
#include "spark_wiring_usbserial.h"
#include "ifapi.h"
#include "scope_guard.h"
#include "check.h"

namespace particle {

enum class TetherInterface {
    NONE,
    USART,
    USB
};

struct TetherSerialConfig {
    TetherSerialConfig();

    TetherSerialConfig& serial(USARTSerial& s);
    USARTSerial& serial() const;

    TetherSerialConfig& serial(USBSerial& s);
    USBSerial& usbserial() const;

    TetherSerialConfig& config(unsigned conf);
    unsigned config() const;

    TetherSerialConfig& baudrate(unsigned baud);
    unsigned baudrate() const;

    TetherInterface interface() const;

private:
    USARTSerial& serial_;
    unsigned config_;
    unsigned baudrate_;

    USBSerial& usbSerial_;
    TetherInterface enabledInterface_ = TetherInterface::NONE;
};

class TetherClass : public spark::NetworkClass {
public:
    TetherClass() :
            NetworkClass(NETWORK_INTERFACE_PPP_SERVER) {
    }

    IPAddress localIP() {
        IPAddress addr;
        GET_IF_ADDR(NETWORK_INTERFACE_PPP_SERVER, addr, addr);
        return addr;
    }

    IPAddress subnetMask() {
        IPAddress addr;
        GET_IF_ADDR(NETWORK_INTERFACE_PPP_SERVER, netmask, addr);
        return addr;
    }

    IPAddress gatewayIP() {
        IPAddress addr;
        GET_IF_ADDR(NETWORK_INTERFACE_PPP_SERVER, gw, addr);
        return addr;
    }

    IPAddress dnsServerIP() {
        return IPAddress();
    }

    IPAddress dhcpServerIP() {
        return IPAddress();
    }

    TetherInterface activeSerialInterface() {
        return activeInterface_;
    }

    int bind(const TetherSerialConfig& config);

private:
    TetherInterface activeInterface_ = TetherInterface::NONE;
};

extern TetherClass Tether;

} /* namespace particle */

#endif /* HAL_PLATFORM_PPP_SERVER */
