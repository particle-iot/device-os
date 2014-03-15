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

/* Network.RSSI() - @TimothyBrown - 20140315 */
/* This function returns the WiFi signal strength in -dB. (Returns a negative number on success or zero on error as a single byte signed integer.) */
/* It reads through all entires in the table, continuing even after finding the correct SSID. This prevents stale entries on the next call. */
/* Note: The RSSI is only refreshed every few minutes, so this function doesn't need to be called constantly in the main loop. */
int8_t NetworkClass::RSSI()
{
    uint8_t _loopCount = 0; /* Meet Vari the Varible! */
    int8_t _returnValue = 0; /* Set to 0 to indicate failure if RSSI isn't detected by end of loop. */
    while (_loopCount++ < 16) /* The wlan_ioctl_get_scan_results table can only hold 16 entires so we don't need to loop any more. */
    {
        unsigned char wlan_scan_results_table[50]; /* Stores current entry from results table. */
        char wlan_scan_results_ssid[32]; /* Holds an SSID from the current entry. */
        if(wlan_ioctl_get_scan_results(0, wlan_scan_results_table) != 0) return(0); /* Loads results table entry into working array *and* checks for an error. */
        if (wlan_scan_results_table[0] == 0) break; /* If we've hit the last SSID in the results table, break from the loop. */
        for (int i = 12; i <=43; i++) /* Reads 32 bytes containing an SSID from results_table into results_ssid. */
        {
            int arrayPos = i - 12;
            wlan_scan_results_ssid[arrayPos] = wlan_scan_results_table[i];
        }
        if (*wlan_scan_results_ssid == *ip_config.uaSSID) _returnValue = ((wlan_scan_results_table[8] >> 1) - 127); /* If we've found the currently connected SSID in the results table, we place the RSSI into _returnValue. */
    } 
    if (_returnValue != 0) return(_returnValue); /* If we've got a valid RSSI, return it! :) */
    else return(0); /* If our RSSI isn't valid, return a null value. :( */
}

NetworkClass Network;
