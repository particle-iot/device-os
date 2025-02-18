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
#include "system_network.h"
#include <chrono>

namespace spark {

class NetworkClass;

// Defined as the primary network
extern NetworkClass Network;

#if HAL_PLATFORM_IFAPI

#define GET_IF_ADDR(_ifindex, _addrType, _addr) \
    if_t iface = nullptr; \
    if (!if_get_by_index(_ifindex, &iface)) { \
        if_addrs* ifAddrList = nullptr; \
        if (!if_get_addrs(iface, &ifAddrList)) { \
            SCOPE_GUARD({ \
                if_free_if_addrs(ifAddrList); \
            }); \
            if_addr* ifAddr = nullptr; \
            for (if_addrs* i = ifAddrList; i; i = i->next) { \
                if (i->if_addr->addr->sa_family == AF_INET) { \
                    ifAddr = i->if_addr; \
                    break; \
                } \
            } \
            if (ifAddr) { \
                auto sockAddr = (const sockaddr_in*)ifAddr->_addrType; \
                _addr = (const uint8_t*)(&sockAddr->sin_addr.s_addr); \
            } \
        } \
    }
#endif // HAL_PLATFORM_IFAPI

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

    static constexpr auto DEFAULT_PING_TIMEOUT = 4000;

    int ping(IPAddress remoteIP, unsigned nTries = 1, unsigned timeoutMs = DEFAULT_PING_TIMEOUT);
    int ping(String hostname, unsigned nTries = 1, unsigned timeoutMs = DEFAULT_PING_TIMEOUT);
    int ping(IPAddress remoteIP, std::chrono::milliseconds timeout, unsigned nTries = 1);
    int ping(String hostname, std::chrono::milliseconds timeout, unsigned nTries = 1);

    virtual void connect(unsigned flags = 0);
    virtual void disconnect();
    virtual bool connecting();
    virtual bool ready();

    virtual void on();
    virtual void off();
    virtual bool isOn();
    virtual bool isOff();
    virtual void listen(bool begin = true);
    virtual void setListenTimeout(uint16_t timeout);
    virtual uint16_t getListenTimeout();
    virtual bool listening();
    virtual NetworkClass& prefer(bool prefer = true);
    virtual bool isPreferred();

    operator network_interface_t() const {
        return iface_;
    }

    static NetworkClass& from(network_interface_t nif);

    virtual IPAddress resolve(const char* name, bool flushCache = false);

    explicit NetworkClass(network_interface_t iface)
            : iface_(iface) {
    }

#if HAL_USE_SOCKET_HAL_POSIX
    int setConfig(const particle::NetworkInterfaceConfig& conf);
    particle::NetworkInterfaceConfig getConfig(String profile = String()) const;
    spark::Vector<particle::NetworkInterfaceConfig> getConfigList() const;
#endif // HAL_USE_SOCKET_HAL_POSIX

private:
    network_interface_t iface_;
};


} // spark

#endif
