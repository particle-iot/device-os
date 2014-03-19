/**
 ******************************************************************************
 * @file    spark_wiring_network.h
 * @author  Satish Nair, Timothy Brown
 * @version V1.0.3
 * @date    18-Mar-2014
 * @brief   Header for spark_wiring_network.cpp module
 ******************************************************************************
  Copyright (c) 2014 Spark Labs, Inc.  All rights reserved.

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

class NetworkClass
{
public:
	NetworkClass();

	uint8_t* macAddress(uint8_t* mac);
	IPAddress localIP();
	IPAddress subnetMask();
	IPAddress gatewayIP();
	char* SSID();
	int8_t RSSI();

	friend class TCPClient;
	friend class TCPServer;

private:
	uint32_t _functionStart;
	uint8_t _loopCount;
	int8_t _returnValue;
};

extern NetworkClass Network;

#endif
