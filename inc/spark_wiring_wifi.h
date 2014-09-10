/**
 ******************************************************************************
 * @file    spark_wiring_wifi.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    7-Mar-2014
 * @brief   Header for spark_wiring_wifi.cpp module
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

#ifndef __SPARK_WIRING_WIFI_H
#define __SPARK_WIRING_WIFI_H

#include "spark_wiring.h"

#define UNSEC   (WLAN_SEC_UNSEC)
#define WEP     (WLAN_SEC_WEP)
#define WPA     (WLAN_SEC_WPA)
#define WPA2    (WLAN_SEC_WPA2)

class IPAddress;

class WiFiClass
{
public:
	WiFiClass() {}
	~WiFiClass() {}

        uint8_t* macAddress(uint8_t* mac);
        IPAddress localIP();
        IPAddress subnetMask();
        IPAddress gatewayIP();
        char* SSID();
        int8_t RSSI();
        uint32_t ping(IPAddress remoteIP);
        uint32_t ping(IPAddress remoteIP, uint8_t nTries);

        static void connect(void);
        static void disconnect(void);
        static bool connecting(void);
        static bool ready(void);
        static void on(void);
        static void off(void);
        static void listen(void);
        static bool listening(void);
        static void setCredentials(const char *ssid);
        static void setCredentials(const char *ssid, const char *password);
        static void setCredentials(const char *ssid, const char *password, unsigned long security);
        static void setCredentials(char *ssid, unsigned int ssidLen, char *password, unsigned int passwordLen, unsigned long security);
        static bool hasCredentials(void);
        static bool clearCredentials(void);

        friend class TCPClient;
        friend class TCPServer;

private:
        uint32_t _functionStart;
        uint8_t _loopCount;
        int8_t _returnValue;
};

extern WiFiClass WiFi;

#endif
