/**
 ******************************************************************************
 * @file    spark_wiring_wifi.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    7-Mar-2013
 * @brief   WiFi utility class to help users manage the WiFi connection
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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

#include "spark_wiring_wifi.h"

void WiFiClass::on(void)
{
	extern void (*announce_presence)(void);
	if(announce_presence != Multicast_Presence_Announcement)
	{
		//Get the setup executed once if not done already
		SPARK_WLAN_Setup(Multicast_Presence_Announcement);
		SPARK_WLAN_SETUP = 1;
	}
	SPARK_WLAN_SLEEP = 0;	//Logic to call wlan_start() inside SPARK_WLAN_Loop()
}

void WiFiClass::off(void)
{
	SPARK_WLAN_SLEEP = 1;	//Logic to call wlan_stop() inside SPARK_WLAN_Loop()
}

WiFi_Status_TypeDef WiFiClass::status(void)
{
	if(SPARK_WLAN_STARTED)
	{
		if(!WLAN_DHCP)
		{
			return WIFI_CONNECTING;
		}
		return WIFI_ON;
	}
	return WIFI_OFF;
}

WiFiClass WiFi;
