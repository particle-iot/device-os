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
#include "ifapi.h"
#include "scope_guard.h"
#include "check.h"

namespace particle {

struct TetherSerialConfig {
    TetherSerialConfig();

    TetherSerialConfig& serial(USARTSerial& s);
    USARTSerial& serial() const;

    TetherSerialConfig& config(unsigned conf);
    unsigned config() const;

    TetherSerialConfig& baudrate(unsigned baud);
    unsigned baudrate() const;

private:
    USARTSerial& serial_;
    unsigned config_;
    unsigned baudrate_;
};

class TetherClass : public spark::NetworkClass {
public:
    TetherClass() :
            NetworkClass(NETWORK_INTERFACE_PPP_SERVER) {
    }

    void on() {
        network_on(*this, 0, 0, NULL);
    }

    void off() {
        network_off(*this, 0, 0, NULL);
    }

    void connect(unsigned flags=0) {
        network_connect(*this, flags, 0, NULL);
    }

    bool connecting(void) {
        return network_connecting(*this, 0, NULL);
    }

    void disconnect() {
        network_disconnect(*this, NETWORK_DISCONNECT_REASON_USER, NULL);
    }

    void listen(bool begin=true) {
        network_listen(*this, begin ? 0 : 1, NULL);
    }

    void setListenTimeout(uint16_t timeout) {
        network_set_listen_timeout(*this, timeout, NULL);
    }

    uint16_t getListenTimeout(void) {
        return network_get_listen_timeout(*this, 0, NULL);
    }

    bool listening(void) {
        return network_listening(*this, 0, NULL);
    }

    bool ready() {
        return network_ready(*this, 0,  NULL);
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

    int bind(const TetherSerialConfig& config);
};

extern TetherClass Tether;

} /* namespace particle */

#endif /* HAL_PLATFORM_PPP_SERVER */
