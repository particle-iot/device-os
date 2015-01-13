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
#include "wlan_hal.h"
#include "spark_wlan.h"
#include "inet_hal.h"

class IPAddress;

namespace spark { 
        
enum SecurityType {
    UNSEC = WLAN_SEC_UNSEC,
    WEP = WLAN_SEC_WEP,
    WPA = WLAN_SEC_WPA,
    WPA2 = WLAN_SEC_WPA2        
};
    
class WiFiClass
{
    
public:
    WiFiClass() {}
    ~WiFiClass() {}

    uint8_t* macAddress(uint8_t *mac) {
        memcpy(mac, network_config().uaMacAddr, 6);
        return mac;
    }

    IPAddress localIP() {
        return IPAddress(network_config().aucIP);
    }

    IPAddress subnetMask() {
        return IPAddress(network_config().aucSubnetMask);
    }

    IPAddress gatewayIP() {
        return IPAddress(network_config().aucDefaultGateway);
    }

    const char *SSID() {
        return (const char *) network_config().uaSSID;
    }

    int8_t RSSI();
    uint32_t ping(IPAddress remoteIP) {
        return ping(remoteIP, 5);
    }

    uint32_t ping(IPAddress remoteIP, uint8_t nTries) {
        return inet_ping(remoteIP.raw_address(), nTries);
    }

    void connect(void) {
        network_connect();
    }

    void disconnect(void) {
        network_disconnect();
    }

    bool connecting(void) {
        return network_connecting();
    }

    bool ready(void) {
        return network_ready();
    }

    void on(void) {
        network_on();
    }

    void off(void) {
        network_off();
    }

    void listen(void) {
        network_listen();
    }

    bool listening(void) {
        return network_listening();
    }

    void setCredentials(const char *ssid) {
        setCredentials(ssid, NULL, UNSEC);
    }

    void setCredentials(const char *ssid, const char *password) {
        setCredentials(ssid, password, WPA2);
    }

    void setCredentials(const char *ssid, const char *password, unsigned long security) {
        setCredentials(ssid, strlen(ssid), password, strlen(password), security);
    }

    void setCredentials(const char *ssid, unsigned int ssidLen, const char *password,
            unsigned int passwordLen, unsigned long security) {
        
        NetworkCredentials creds;
        creds.ssid = ssid;
        creds.ssidLen = ssidLen;
        creds.password = password;
        creds.passwordLen = passwordLen;
        creds.security = security;
        
        network_set_credentials(&creds);
    }

    bool hasCredentials(void) {
        return wlan_has_credentials() == 0;
    }

    bool clearCredentials(void) {
        return wlan_clear_credentials() == 0;
    }

    friend class TCPClient;
    friend class TCPServer;

};

extern WiFiClass WiFi;

}   // namespace Spark

#endif
