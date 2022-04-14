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

#ifndef SPARK_WIRING_ETHERNET_H
#define SPARK_WIRING_ETHERNET_H

#include "spark_wiring_platform.h"

#if Wiring_Ethernet

#include "spark_wiring_network.h"
#include "system_network.h"
#include "ifapi.h"
#include "scope_guard.h"
#include "check.h"

namespace spark {

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

class EthernetClass : public NetworkClass {
public:
    EthernetClass() :
            NetworkClass(NETWORK_INTERFACE_ETHERNET) {
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

    uint8_t* macAddress(uint8_t* mac) {
        if_t iface = nullptr;
        if (!if_get_by_index(NETWORK_INTERFACE_ETHERNET, &iface)) {
            sockaddr_ll hwAddr = {};
            if (!if_get_lladdr(iface, &hwAddr)) {
                memcpy(mac, hwAddr.sll_addr, 6);
                return mac;
            }
        }
        return nullptr;
    }

    IPAddress localIP() {
        IPAddress addr;
        GET_IF_ADDR(NETWORK_INTERFACE_ETHERNET, addr, addr);
        return addr;
    }

    IPAddress subnetMask() {
        IPAddress addr;
        GET_IF_ADDR(NETWORK_INTERFACE_ETHERNET, netmask, addr);
        return addr;
    }

    IPAddress gatewayIP() {
        IPAddress addr;
        GET_IF_ADDR(NETWORK_INTERFACE_ETHERNET, gw, addr);
        return addr;
    }

    IPAddress dnsServerIP() {
        return IPAddress();
    }

    IPAddress dhcpServerIP() {
        return IPAddress();
    }
};


extern EthernetClass Ethernet;

} /* namespace spark */

#endif /* Wiring_Ethernet */
#endif /* SPARK_WIRING_ETHERNET_H */
