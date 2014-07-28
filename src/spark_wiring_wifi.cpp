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
#include "spark_wiring_network.h"

// WiFi.on() is deprecated and will be removed soon
void WiFiClass::on(void)
{
  Network.connect();
}

// WiFi.off() is deprecated and will be removed soon
void WiFiClass::off(void)
{
  Network.disconnect();
}

void WiFiClass::listen(void)
{
  WLAN_SMART_CONFIG_START = 1;
}

bool WiFiClass::listening(void)
{
  if (WLAN_SMART_CONFIG_START && !(WLAN_SMART_CONFIG_FINISHED || WLAN_SERIAL_CONFIG_DONE))
  {
    return true;
  }

  return false;
}

void WiFiClass::setCredentials(const char *ssid)
{
  setCredentials(ssid, NULL, UNSEC);
}

void WiFiClass::setCredentials(const char *ssid, const char *password)
{
  setCredentials(ssid, password, WPA2);
}

void WiFiClass::setCredentials(const char *ssid, const char *password, unsigned long security)
{
  setCredentials((char *)ssid, strlen(ssid), (char *)password, strlen(password), security);
}

void WiFiClass::setCredentials(char *ssid, unsigned int ssidLen, char *password, unsigned int passwordLen, unsigned long security)
{
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
        ssidLen,                                              // SSID length
        NULL,                                                 // BSSID
        1,                                                    // Priority
        0, 0, 0, 0, 0);

      break;
    }

  case WLAN_SEC_WEP://WEP
    {
      if(!WLAN_SMART_CONFIG_FINISHED)
      {
        // Get WEP key from string, needs converting
        passwordLen = (strlen(password)/2); // WEP key length in bytes
        char byteStr[3]; byteStr[2] = '\0';

        for (UINT32 i = 0 ; i < passwordLen ; i++) { // Basic loop to convert text-based WEP key to byte array, can definitely be improved
          byteStr[0] = password[2*i]; byteStr[1] = password[(2*i)+1];
          password[i] = strtoul(byteStr, NULL, 16);
        }
      }

      wlan_profile_index = wlan_add_profile(WLAN_SEC_WEP,    // Security type
        (unsigned char *)ssid,                                // SSID
        ssidLen,                                              // SSID length
        NULL,                                                 // BSSID
        1,                                                    // Priority
        passwordLen,                                          // KEY length
        0,                                                    // KEY index
        0,
        (unsigned char *)password,                            // KEY
        0);

      break;
    }

  case WLAN_SEC_WPA://WPA
  case WLAN_SEC_WPA2://WPA2
    {
      wlan_profile_index = wlan_add_profile(WLAN_SEC_WPA2,    // Security type
        (unsigned char *)ssid,                                // SSID
        ssidLen,                                              // SSID length
        NULL,                                                 // BSSID
        1,                                                    // Priority
        0x18,                                                 // PairwiseCipher
        0x1e,                                                 // GroupCipher
        2,                                                    // KEY management
        (unsigned char *)password,                            // KEY
        passwordLen);                                         // KEY length

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
  if(NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET] != 0)
  {
    return true;
  }

  return false;
}

void WiFiClass::clearCredentials(void)
{
  if(wlan_ioctl_del_profile(255) == 0)
  {
    extern void recreate_spark_nvmem_file(void);
    recreate_spark_nvmem_file();
  }
}

WiFiClass WiFi;
