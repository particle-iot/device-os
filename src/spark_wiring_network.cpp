/**
 ******************************************************************************
 * @file    spark_wiring_network.cpp
 * @author  Satish Nair, Timothy Brown
 * @version V1.0.1
 * @date    15-Mar-2014
 * @brief   
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

/*	***********************************************	*/
/*	* Network.RSSI() - @TimothyBrown - 2014.03.17 *	*/
/*	***********************************************	*/
/*	----------------------------------------------------------------------------------------------------------------------------------------------	*/
/*		This function returns the WiFi signal strength in -dB [-127 to 0]. Error Values: 1 [CC3000 Problem]; 2 [Function Timeout/Bad Data].			*/
/*		It reads through all entires in the table, continuing even after finding the correct SSID; this prevents stale entries on the next call.	*/
/*		There is a one second timeout on the function for safety; this also forces the function to re-run if the results table is corrupt.			*/
/*		Note: The CC3000 only updates the RSSI status every few minutes, so this function doesn't need to be called constantly in the main loop.	*/
/*	----------------------------------------------------------------------------------------------------------------------------------------------	*/
int8_t NetworkClass::RSSI()
{
	_functionTimeout = millis() + 1000;
	_loopCount = 0;
	_returnValue = 0;
	while (millis() < _functionTimeout)
	{
		while (_loopCount++ < 16)
		{
			unsigned char wlan_scan_results_table[50];
			char wlan_scan_results_ssid[32];
			if(wlan_ioctl_get_scan_results(0, wlan_scan_results_table) != 0) return(1);
			for (int i = 12; i <= 43; i++)
			{
				int arrayPos = i - 12;
				wlan_scan_results_ssid[arrayPos] = wlan_scan_results_table[i];
			}
			if (*wlan_scan_results_ssid == *ip_config.uaSSID) _returnValue = ((wlan_scan_results_table[8] >> 1) - 127);
			if (wlan_scan_results_table[0] == 0) break;
		}
		if (_returnValue != 0) return(_returnValue);
	}
	return(2);
}

NetworkClass Network;
