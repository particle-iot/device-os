/**
 ******************************************************************************
 * @file    spark_wiring_network.h
 * @author  Satish Nair, Timothy Brown
 * @version V1.0.0
 * @date    18-Mar-2014
 * @brief   Header for spark_wiring_network.cpp module
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

#ifndef __SPARK_WIRING_NETWORK_H
#define __SPARK_WIRING_NETWORK_H

#include "spark_wiring_ipaddress.h"

namespace spark {

class NetworkClass;

// Defined as the primary network
extern NetworkClass Network;

//Retained for compatibility and to flag compiler warnings as build errors
class NetworkClass
{
public:
    uint8_t* macAddress(uint8_t* mac) __attribute__((deprecated("Please use WiFi.macAddress() instead")));
    IPAddress localIP() __attribute__((deprecated("Please use WiFi.localIP() instead")));
    IPAddress subnetMask() __attribute__((deprecated("Please use WiFi.subnetMask() instead")));
    IPAddress gatewayIP() __attribute__((deprecated("Please use WiFi.gatewayIP() instead")));
    char* SSID() __attribute__((deprecated("Please use WiFi.SSID() instead")));
    int8_t RSSI() __attribute__((deprecated("Please use WiFi.RSSI() instead")));
    uint32_t ping(IPAddress remoteIP) __attribute__((deprecated("Please use WiFi.ping() instead")));
    uint32_t ping(IPAddress remoteIP, uint8_t nTries) __attribute__((deprecated("Please use WiFi.ping() instead")));

    virtual void connect(unsigned flags = 0);
    virtual void disconnect();
    virtual bool connecting();
    virtual bool ready();

    virtual void on();
    virtual void off();
    virtual void listen(bool begin = true);
    virtual void setListenTimeout(uint16_t timeout);
    virtual uint16_t getListenTimeout();
    virtual bool listening();

    operator network_interface_t() {
        return iface_;
    }

    static NetworkClass& from(network_interface_t nif);

    virtual IPAddress resolve(const char* name);

    explicit NetworkClass(network_interface_t iface)
            : iface_(iface) {
    }

private:
    network_interface_t iface_;
};


} // spark

#endif
