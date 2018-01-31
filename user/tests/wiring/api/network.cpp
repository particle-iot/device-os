/**
 ******************************************************************************
 * @file    network.cpp
 * @authors Matthew McGowan
 * @date    13 January 2015
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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


#include "testapi.h"

test(api_ip_address) {

    API_COMPILE(IPAddress(HAL_IPAddress()));



}


test(api_tcpserver) {

    TCPServer server(80);
    int available = 0;
    API_COMPILE(server.begin());
    API_COMPILE(available = server.available());
    API_COMPILE(server.stop());
    (void)available;
}

test(api_udp_multicast) {
    UDP udp;
    udp.begin(10000);
    int result = 0;
    API_COMPILE(result = udp.joinMulticast(IPAddress(224, 1, 2, 3)));
    API_COMPILE(result = udp.leaveMulticast(IPAddress(224, 1, 2, 3)));
    (void)result; // avoid unused warning
}

test(api_udp_direct) {
    UDP udp;
    uint8_t buf[50];
    API_COMPILE(udp.setBuffer(1024, buf));
    API_COMPILE(udp.setBuffer(1024));
    API_COMPILE(udp.releaseBuffer());
    API_COMPILE(udp.sendPacket("hello", 5, IPAddress(1,2,3,4), 50));
    API_COMPILE(udp.sendPacket(new uint8_t[5], 5, IPAddress(1,2,3,4), 50));

    API_COMPILE(udp.receivePacket(new uint8_t[5], 5));
}

test(api_tcpserver_write_timeout) {
    TCPServer server(1000);
    API_COMPILE(server.write(0xff, 123456));
    API_COMPILE(server.write((const uint8_t*)&server, sizeof(server), 123456));
}

test(api_tcpclient_write_timeout) {
    TCPClient client;
    API_COMPILE(client.write(0xff, 123456));
    API_COMPILE(client.write((const uint8_t*)&client, sizeof(client), 123456));
}
