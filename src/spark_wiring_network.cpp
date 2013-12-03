/**
 ******************************************************************************
 * @file    spark_wiring_network.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    10-Nov-2013
 * @brief   
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

#include "spark_wiring_network.h"

NetworkClass::NetworkClass()
{
}

uint8_t* NetworkClass::macAddress(uint8_t* mac)
{
	memcpy(mac, ip_config.uaMacAddr, 6);
	return mac;
}

IPAddress NetworkClass::localIP()
{
	return IPAddress(ip_config.aucIP[3], ip_config.aucIP[2], ip_config.aucIP[1], ip_config.aucIP[0]);
}

IPAddress NetworkClass::subnetMask()
{
	return IPAddress(ip_config.aucSubnetMask[3], ip_config.aucSubnetMask[2], ip_config.aucSubnetMask[1], ip_config.aucSubnetMask[0]);
}

IPAddress NetworkClass::gatewayIP()
{
	return IPAddress(ip_config.aucDefaultGateway[3], ip_config.aucDefaultGateway[2], ip_config.aucDefaultGateway[1], ip_config.aucDefaultGateway[0]);
}

char* NetworkClass::SSID()
{
	return (char *)ip_config.uaSSID;
}

NetworkClass Network;
