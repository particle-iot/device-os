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
#include "inet_hal.h"
#include "rgbled.h"
#include "spark_wlan.h"

namespace spark {

    uint8_t *WiFiClass::macAddress(uint8_t *mac) {
        memcpy(mac, ip_config.uaMacAddr, 6);
        return mac;
    }

    IPAddress WiFiClass::localIP() {
        return IPAddress(ip_config.aucIP);
    }

    IPAddress WiFiClass::subnetMask() {
        return IPAddress(ip_config.aucSubnetMask);
    }

    IPAddress WiFiClass::gatewayIP() {
        return IPAddress(ip_config.aucDefaultGateway);
    }

    const char *WiFiClass::SSID() {
        return (const char *) ip_config.uaSSID;
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

    int8_t WiFiClass::RSSI() {
        _functionStart = millis();
        while ((millis() - _functionStart) < 1000) {
            int rv = wlan_connected_rssi();
            if (rv != 0)
                return (rv);
        }
        return (2);
    }

/********************************* Bug Notice *********************************
On occasion, "wlan_ioctl_get_scan_results" only returns a single bad entry
(with index 0). I suspect this happens when the CC3000 is refreshing the
scan table; I think it deletes the current entries, does a new scan then
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

    uint32_t WiFiClass::ping(IPAddress remoteIP) {
        return ping(remoteIP, 5);
    }

    uint32_t WiFiClass::ping(IPAddress remoteIP, uint8_t nTries) {
        return inet_ping(remoteIP.raw_address(), nTries);
    }

    void WiFiClass::connect(void) {
        if (!ready()) {
            WLAN_DISCONNECT = 0;
            SPARK_WLAN_STARTED = 1;
            SPARK_WLAN_SLEEP = 0;

            wlan_connect_init();

            if (wlan_reset_credentials_store_required()) {
                wlan_reset_credentials_store();
            }

            if (!WLAN_MANUAL_CONNECT && !WiFi.hasCredentials()) {
                WiFi.listen();
            }
            else {
                SPARK_LED_FADE = 0;
                    LED_SetRGBColor(RGB_COLOR_GREEN);
                LED_On(LED_RGB);
                wlan_connect_finalize();
            }

            Set_NetApp_Timeout();
        }
    }

    void WiFiClass::disconnect(void) {
        if (ready()) {
            WLAN_DISCONNECT = 1;//Do not ARM_WLAN_WD() in WLAN_Async_Callback()
            SPARK_CLOUD_CONNECT = 0;
            wlan_disconnect_now();
        }
    }

    bool WiFiClass::connecting(void) {
        return (SPARK_WLAN_STARTED && !WLAN_DHCP);
    }

    bool WiFiClass::ready(void) {
        return (SPARK_WLAN_STARTED && WLAN_DHCP);
    }

    void WiFiClass::on(void) {
        if (!SPARK_WLAN_STARTED) {
            wlan_activate();
            SPARK_WLAN_STARTED = 1;
            SPARK_WLAN_SLEEP = 0;
            SPARK_LED_FADE = 1;
            LED_SetRGBColor(RGB_COLOR_BLUE);
            LED_On(LED_RGB);
        }
    }

    void WiFiClass::off(void) {
        if (SPARK_WLAN_STARTED) {
            wlan_deactivate();

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

    void WiFiClass::listen(void) {
        WLAN_SMART_CONFIG_START = 1;
    }

    bool WiFiClass::listening(void) {
        if (WLAN_SMART_CONFIG_START && !(WLAN_SMART_CONFIG_FINISHED || WLAN_SERIAL_CONFIG_DONE)) {
            return true;
        }
        return false;
    }

    void WiFiClass::setCredentials(const char *ssid) {
        setCredentials(ssid, NULL, UNSEC);
    }

    void WiFiClass::setCredentials(const char *ssid, const char *password) {
        setCredentials(ssid, password, WPA2);
    }

    void WiFiClass::setCredentials(const char *ssid, const char *password, unsigned long security) {
        setCredentials(ssid, strlen(ssid), password, strlen(password), security);
    }

    void WiFiClass::setCredentials(const char *ssid, unsigned int ssidLen, const char *password,
            unsigned int passwordLen, unsigned long security) {
        if (!SPARK_WLAN_STARTED) {
            return;
        }

        if (0 == password[0]) {
            security = WLAN_SEC_UNSEC;
        }

        char buf[14];
        if (security == WLAN_SEC_WEP && !WLAN_SMART_CONFIG_FINISHED) {
            // Get WEP key from string, needs converting
            passwordLen = (strlen(password) / 2); // WEP key length in bytes
            char byteStr[3];
            byteStr[2] = '\0';
            memset(buf, 0, sizeof(buf));
            for (uint32_t i = 0; i < passwordLen; i++) { // Basic loop to convert text-based WEP key to byte array, can definitely be improved
                byteStr[0] = password[2 * i];
                byteStr[1] = password[(2 * i) + 1];
                buf[i] = strtoul(byteStr, NULL, 16);
            }
            password = buf;
        }

        wlan_set_credentials(ssid, ssidLen, password, passwordLen, WLanSecurityType(security));
    }

    bool WiFiClass::hasCredentials(void) {
        return wlan_has_credentials() != 0;
    }

    bool WiFiClass::clearCredentials(void) {
        return wlan_clear_credentials() != 0;
    }

    WiFiClass WiFi;

}