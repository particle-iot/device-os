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

uint8_t* WiFiClass::macAddress(uint8_t* mac)
{
        memcpy(mac, ip_config.uaMacAddr, 6);
        return mac;
}

IPAddress WiFiClass::localIP()
{
        return IPAddress(ip_config.aucIP[3], ip_config.aucIP[2], ip_config.aucIP[1], ip_config.aucIP[0]);
}

IPAddress WiFiClass::subnetMask()
{
        return IPAddress(ip_config.aucSubnetMask[3], ip_config.aucSubnetMask[2], ip_config.aucSubnetMask[1], ip_config.aucSubnetMask[0]);
}

IPAddress WiFiClass::gatewayIP()
{
        return IPAddress(ip_config.aucDefaultGateway[3], ip_config.aucDefaultGateway[2], ip_config.aucDefaultGateway[1], ip_config.aucDefaultGateway[0]);
}

char* WiFiClass::SSID()
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

int8_t WiFiClass::RSSI()
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

uint32_t WiFiClass::ping(IPAddress remoteIP)
{
  return ping(remoteIP, 5);
}

uint32_t WiFiClass::ping(IPAddress remoteIP, uint8_t nTries)
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

void WiFiClass::connect(void)
{
  if(!ready())
  {
    WLAN_DISCONNECT = 0;
    wlan_start(0);//No other option to connect other than wlan_start()
    SPARK_WLAN_STARTED = 1;
    SPARK_WLAN_SLEEP = 0;

    /* Mask out all non-required events from CC3000 */
    wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE | HCI_EVNT_WLAN_UNSOL_INIT);

    if(NVMEM_SPARK_Reset_SysFlag == 0x0001 || nvmem_read(NVMEM_SPARK_FILE_ID, NVMEM_SPARK_FILE_SIZE, 0, NVMEM_Spark_File_Data) != NVMEM_SPARK_FILE_SIZE)
    {
      /* Delete all previously stored wlan profiles */
      WiFi.clearCredentials();

      NVMEM_SPARK_Reset_SysFlag = 0x0000;
      Save_SystemFlags();
    }

    if(!WLAN_MANUAL_CONNECT && !WiFi.hasCredentials())
    {
      WiFi.listen();
    }
    else
    {
      SPARK_LED_FADE = 0;
      LED_SetRGBColor(RGB_COLOR_GREEN);
      LED_On(LED_RGB);
      wlan_ioctl_set_connection_policy(DISABLE, DISABLE, ENABLE);//Enable auto connect
    }

    Set_NetApp_Timeout();
  }
}

void WiFiClass::disconnect(void)
{
  if(ready())
  {
    WLAN_DISCONNECT = 1;//Do not ARM_WLAN_WD() in WLAN_Async_Callback()
    SPARK_CLOUD_CONNECT = 0;
    wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);//Disable auto connect
    wlan_disconnect();
  }
}

bool WiFiClass::connecting(void)
{
  if(SPARK_WLAN_STARTED && !WLAN_DHCP)
  {
    return true;
  }
  return false;
}

bool WiFiClass::ready(void)
{
  if(SPARK_WLAN_STARTED && WLAN_DHCP)
  {
    return true;
  }
  return false;
}

void WiFiClass::on(void)
{
  if(!SPARK_WLAN_STARTED)
  {
    wlan_start(0);
    SPARK_WLAN_STARTED = 1;
    SPARK_WLAN_SLEEP = 0;
    SPARK_LED_FADE = 1;
    LED_SetRGBColor(RGB_COLOR_BLUE);
    LED_On(LED_RGB);
    wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);//Disable auto connect
  }
}

void WiFiClass::off(void)
{
  if(SPARK_WLAN_STARTED)
  {
    wlan_stop();

    // Reset remaining state variables in SPARK_WLAN_Loop()
    SPARK_WLAN_SLEEP = 1;

    // Do not automatically connect to the cloud
    // the next time we connect to a Wi-Fi network
    SPARK_CLOUD_CONNECT = 0;

    SPARK_LED_FADE = 1;
    LED_SetRGBColor(RGB_COLOR_WHITE);
    LED_On(LED_RGB);
  }
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

bool WiFiClass::clearCredentials(void)
{
  if(wlan_ioctl_del_profile(255) == 0)
  {
    extern void recreate_spark_nvmem_file(void);
    recreate_spark_nvmem_file();
    Clear_NetApp_Dhcp();
    return true;
  }
  return false;
}

WiFiClass WiFi;
