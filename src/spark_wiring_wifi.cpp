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

void WiFiClass::listen(void)
{
  //Work in Progress
}

bool WiFiClass::listening(void)
{
  //Work in Progress
  return false;
}

void WiFiClass::setCredentials(const char *ssid)
{
  //Work in Progress
  setCredentials(ssid, NULL, UNSEC);
}

void WiFiClass::setCredentials(const char *ssid, const char *password)
{
  //Work in Progress
  setCredentials(ssid, password, WPA2);
}

void WiFiClass::setCredentials(const char *ssid, const char *password, unsigned long security)
{
  //Work in Progress
  if(!SPARK_WLAN_STARTED)
  {
    return;
  }

  if (0 == password[0]) {
    security = WLAN_SEC_UNSEC;
  }

  // add a profile
  switch (security)
  {
  case WLAN_SEC_UNSEC://None
    {
      wlan_profile_index = wlan_add_profile(WLAN_SEC_UNSEC,   // Security type
        (unsigned char *)ssid,                                // SSID
        strlen(ssid),                                         // SSID length
        NULL,                                                 // BSSID
        1,                                                    // Priority
        0, 0, 0, 0, 0);

      break;
    }

  case WLAN_SEC_WEP://WEP
    {
      // Get WEP key from string, needs converting
      UINT32 keyLen = (strlen(password)/2); // WEP key length in bytes
      UINT8 decKey[32]; // Longest WEP key I can find is 256-bit, or 32 bytes long
      char byteStr[3]; byteStr[2] = '\0';

      for (UINT32 i = 0 ; i < keyLen ; i++) { // Basic loop to convert text-based WEP key to byte array, can definitely be improved
        byteStr[0] = password[2*i]; byteStr[1] = password[(2*i)+1];
        decKey[i] = strtoul(byteStr, NULL, 16);
      }

      wlan_profile_index = wlan_add_profile(WLAN_SEC_WEP,    // Security type
        (unsigned char *)ssid,                                // SSID
        strlen(ssid),                                         // SSID length
        NULL,                                                 // BSSID
        1,                                                    // Priority
        keyLen,                                               // KEY length
        0,                                                    // KEY index
        0,
        decKey,                                               // KEY
        0);

      break;
    }

  case WLAN_SEC_WPA://WPA
  case WLAN_SEC_WPA2://WPA2
    {
      wlan_profile_index = wlan_add_profile(WLAN_SEC_WPA2,    // Security type
        (unsigned char *)ssid,                                // SSID
        strlen(ssid),                                         // SSID length
        NULL,                                                 // BSSID
        1,                                                    // Priority
        0x18,                                                 // PairwiseCipher
        0x1e,                                                 // GroupCipher
        2,                                                    // KEY management
        (unsigned char *)password,                            // KEY
        strlen(password));                                    // KEY length

      break;
    }
  }

  if(wlan_profile_index != -1)
  {
    NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET] = wlan_profile_index + 1;
  }

  /* write count of wlan profiles stored */
  nvmem_write(NVMEM_SPARK_FILE_ID, 1, WLAN_PROFILE_FILE_OFFSET, &NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET]);
}

bool WiFiClass::hasCredentials(void)
{
  //Work in Progress
  return false;
}

void WiFiClass::clearCredentials(void)
{
  //Work in Progress
}

//Duplicated as Network.connect()
//Retained for backward compatibility
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

//Duplicated as Network.disconnect()
//Retained for backward compatibility
void WiFiClass::off(void)
{
	SPARK_WLAN_SLEEP = 1;	//Logic to call wlan_stop() inside SPARK_WLAN_Loop()
}

//Duplicated as Network.connectinging() and Network.ready()
//Retained for backward compatibility
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
