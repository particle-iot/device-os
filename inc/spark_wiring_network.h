/**
 ******************************************************************************
 * @file    spark_wiring_network.h
 * @author  Satish Nair, Timothy Brown
 * @version V1.0.0
 * @date    18-Mar-2014
 * @brief   Header for spark_wiring_network.cpp module
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

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

#include "spark_wiring.h"

//Retained for compatibility and to flag compiler warnings as build errors
class NetworkClass
{
public:
	uint8_t* macAddress(uint8_t* mac) __attribute__((deprecated));
	IPAddress localIP() __attribute__((deprecated));
	IPAddress subnetMask() __attribute__((deprecated));
	IPAddress gatewayIP() __attribute__((deprecated));
	char* SSID() __attribute__((deprecated));
	int8_t RSSI() __attribute__((deprecated));
	uint32_t ping(IPAddress remoteIP) __attribute__((deprecated));
	uint32_t ping(IPAddress remoteIP, uint8_t nTries) __attribute__((deprecated));

        static void connect(void) __attribute__((deprecated));
        static void disconnect(void) __attribute__((deprecated));
        static bool connecting(void) __attribute__((deprecated));
        static bool ready(void) __attribute__((deprecated));
};

extern NetworkClass Network;

#endif
