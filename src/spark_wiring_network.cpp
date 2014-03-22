/**
 ******************************************************************************
 * @file    spark_wiring_network.cpp
 * @author  Satish Nair, Timothy Brown
 * @version V1.0.0
 * @date    18-Mar-2014
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

/* ***********************************************
   * Network.RSSI() - @TimothyBrown - 2014.03.18 *
   ***********************************************
   -----------------------------------------------
    Command: Network.RSSI()
    Returns: Signal Strength from -127 to -1dB
    Errors:  [1]CC300 Issue; [2]Function Timeout
    Timeout: One Second
   ----------------------------------------------- */

int8_t NetworkClass::RSSI()
{
	_functionStart = millis();
	_returnValue = 0;
	while ((millis() - _functionStart) < 1000)
	{
		_loopCount = 0;
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

/********************************* Bug Notice *********************************
 On occasion, "wlan_ioctl_get_scan_results" only returns a single bad entry
 (with index 0). I suspect this happens when the CC3000 is refreshing the
 scan table; I suspect this happens when the CC3000 is refreshing the scan
 table; I think it deletes the current entires, does a new scan then
 repopulates the table. If the function is called during this process
 the table only contains the invalid zero indexed entry.
 The good news is the way I've designed the function mitigates this problem.
 The main while loop prevents the function from running for more than one
 second; the inner while loop prevents the function from reading more than
 16 entries from the scan table (which is the maximum amount it can hold).
 The first byte of the scan table lists the number of entries remaining;
 we use this to break out of the inner loop when we reach the last entry.
 This is done so that we read out the entire scan table (ever after finding
 our SSID) so the data isn't stale on the next function call. If the function
 is called when the table contains invalid data, the index will be zero;
 this causes the inner loop to break and start again; this action will
 repeat until the scan table has been repopulated with valid entries (or the
 one second timeout is reached). If the aforementioned "bug" is ever fixed by
 TI, no changes need to be made to this function, as it would be implemented
 the same way.
 *****************************************************************************/

uint32_t NetworkClass::ping(IPAddress remoteIP)
{
  return ping(remoteIP, 5);
}

uint32_t NetworkClass::ping(IPAddress remoteIP, uint8_t nTries)
{
  uint32_t result = 0;
  uint32_t pingIPAddr = remoteIP[3] << 24 | remoteIP[2] << 16 | remoteIP[1] << 8 | remoteIP[0];
  unsigned long pingSize = 32UL;
  unsigned long pingTimeout = 500UL; // in milliseconds

  memset(&ping_report,0,sizeof(netapp_pingreport_args_t));
  ping_report_num = 0;	 

  long psend = netapp_ping_send(&pingIPAddr, (unsigned long)nTries, pingSize, pingTimeout);
  unsigned long lastTime = millis();
  while( ping_report_num==0 && (millis() < lastTime+2*nTries*pingTimeout)) {}
  if (psend==0L && ping_report_num) {
    result = ping_report.packets_received;
  }
  return result;
}

NetworkClass Network;
