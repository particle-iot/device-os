/**
 ******************************************************************************
 * @file    cloud.cpp
 * @authors Matthew McGowan
 * @date    13 January 2015
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#include "testapi.h"

#if Wiring_WiFi

test(api_wifi_config)
{
	IPAddress address;
	uint8_t mac[6];
	uint8_t* ether;
	String ssid;
    API_COMPILE(ether=WiFi.macAddress(mac));
#if !HAL_PLATFORM_WIFI_SCAN_ONLY
	API_COMPILE(ssid=WiFi.SSID());
	API_COMPILE(address=WiFi.localIP());
	API_COMPILE(address=WiFi.dnsServerIP());
	API_COMPILE(address=WiFi.dhcpServerIP());
	API_COMPILE(address=WiFi.gatewayIP());
	API_COMPILE(ether=WiFi.BSSID(mac));
	(void)ether;
#endif // !HAL_PLATFORM_WIFI_SCAN_ONLY
}

test(api_set_get_country_code)
{
    int rc = 0;
    API_COMPILE(rc=WiFi.setCountryCode(wlan_country_code_t::WLAN_CC_KR));
    API_COMPILE(rc=WiFi.getCountryCode());
    (void)rc;
}


#if !HAL_PLATFORM_WIFI_SCAN_ONLY
test(api_wifi_resolve)
{
    API_COMPILE(WiFi.resolve(String("abc.def.com")));
    API_COMPILE(WiFi.resolve("abc.def.com"));
}

test (api_wifi_on_off)
{
    API_COMPILE(WiFi.on());
    API_COMPILE(WiFi.off());
}

test (api_wifi_is_on_off)
{
    API_COMPILE(WiFi.isOn());
    API_COMPILE(WiFi.isOff());
}

test (api_wifi_connect) {
    bool result;
    API_COMPILE(WiFi.connect());
    API_COMPILE(WiFi.connect(WIFI_CONNECT_SKIP_LISTEN));
    API_COMPILE(result = WiFi.connecting());
    (void)result; // avoid unused warning
}

test (wifi_api_listen)
{
    bool result;
    uint16_t val;
    API_COMPILE(WiFi.listen());
    API_COMPILE(WiFi.listen(false));
    API_COMPILE(result = WiFi.listening());
    API_COMPILE(WiFi.setListenTimeout(10));
    API_COMPILE(WiFi.setListenTimeout(10s));
    API_COMPILE(val = WiFi.getListenTimeout());
    (void)result; // avoid unused warning
    (void)val;    //   |
}

#endif // !HAL_PLATFORM_WIFI_SCAN_ONLY

#if PLATFORM_ID != PLATFORM_GCC && PLATFORM_ID != PLATFORM_NEWHAL
test(api_wifi_selectantenna)
{
    API_COMPILE(WiFi.selectAntenna(ANT_AUTO));
    API_COMPILE(WiFi.selectAntenna(ANT_INTERNAL));
    API_COMPILE(WiFi.selectAntenna(ANT_EXTERNAL));
}
#endif // PLATFORM_ID != PLATFORM_GCC && PLATFORM_ID != PLATFORM_NEWHAL

#if !HAL_PLATFORM_WIFI_SCAN_ONLY
test(api_wifi_set_credentials)
{
    bool ok = false;
    API_COMPILE(ok = WiFi.setCredentials("ssid)",4,"password", 8, WPA2));
    API_COMPILE(ok = WiFi.setCredentials("ssid)",4,"password", 8, WPA2, WLAN_CIPHER_AES));
    API_COMPILE(ok = WiFi.setCredentials("ssid)","password", WPA2, WLAN_CIPHER_AES));
    (void)ok; // avoid unused warning
}

test(api_wifi_set_credentials_obj)
{
    WiFiCredentials cred;
    API_COMPILE(cred.setSsid("ssid)"));
    API_COMPILE(cred.setPassword("password"));
    API_COMPILE(cred.setSecurity(WPA3));
    API_COMPILE(cred.setCipher(WLAN_CIPHER_AES));
    API_COMPILE(cred.setHidden(true));
    API_COMPILE(WiFi.setCredentials(cred));
}


test(api_wifi_set_security)
{
    WiFiCredentials credentials;
    credentials.setSecurity(WPA2);
}

#if !HAL_PLATFORM_NCP
test(api_wifi_setStaticIP)
{
    IPAddress myAddress(192,168,1,100);
    IPAddress netmask(255,255,255,0);
    IPAddress gateway(192,168,1,1);
    IPAddress dns(192,168,1,1);
    WiFi.setStaticIP(myAddress, netmask, gateway, dns);

    // now let's use the configured IP
    WiFi.useStaticIP();
    WiFi.useDynamicIP();
}
#endif // !HAL_PLATFORM_NCP
#endif // !HAL_PLATFORM_WIFI_SCAN_ONLY

test(api_wifi_scan_buffer)
{
    WiFiAccessPoint ap[20];
    int found = WiFi.scan(ap, 20);
    for (int i=0; i<found; i++) {
        Serial.print("ssid: ");
        Serial.println(ap[i].ssid);
        Serial.println(ap[i].security);
        Serial.println(ap[i].channel);
        Serial.println(ap[i].rssi);
    }
}


void wifi_scan_callback(WiFiAccessPoint* wap, void* data)
{
    WiFiAccessPoint& ap = *wap;
    Serial.print("ssid: ");
    Serial.println(ap.ssid);
    Serial.print("security: ");
    Serial.println(ap.security);
    Serial.print("Channel: ");
    Serial.println(ap.channel);
    Serial.print("RSSI: ");
    Serial.println(ap.rssi);
}

test(api_wifi_scan_callback)
{
    int result_count = WiFi.scan(wifi_scan_callback);
    (void)result_count;
}

class FindStrongestSSID
{
    char strongest_ssid[33];
    int strongest_rssi;

    static void handle_ap(WiFiAccessPoint* wap, FindStrongestSSID* self)
    {
        self->next(*wap);
    }

    void next(WiFiAccessPoint& ap)
    {
        if ((ap.rssi < 0) && (ap.rssi > strongest_rssi)) {
            strongest_rssi = ap.rssi;
            strcpy(strongest_ssid, ap.ssid);
        }
    }

public:

    /**
     * Scan WiFi Access Points and retrieve the strongest one.
     */
    const char* find()
    {
        strongest_rssi = 1;
        strongest_ssid[0] = 0;
        WiFi.scan(handle_ap, this);
        return strongest_ssid;
    }
};

test(api_find_strongest)
{
    FindStrongestSSID finder;
    const char* ssid = finder.find();
    (void)ssid;
}

#if !HAL_PLATFORM_WIFI_SCAN_ONLY
test(api_wifi_ipconfig)
{
    IPAddress address;
    API_COMPILE(address=WiFi.localIP());
    API_COMPILE(address=WiFi.gatewayIP());
    API_COMPILE(address=WiFi.dnsServerIP());
    API_COMPILE(address=WiFi.dhcpServerIP());
    (void)address;
}

test(api_wifi_get_credentials)
{
    WiFiAccessPoint ap[10];
    int found = WiFi.getCredentials(ap, 10);
    for (int i=0; i<found; i++) {
        Serial.print("ssid: ");
        Serial.println(ap[i].ssid);
        Serial.println(ap[i].security);
        Serial.println(ap[i].channel);
        Serial.println(ap[i].rssi);
    }
}

#if !HAL_PLATFORM_NCP
test(api_wifi_hostname)
{
    String hostname;
    const char shostname[] = "testhostname";
    API_COMPILE(hostname = WiFi.hostname());
    API_COMPILE(WiFi.setHostname(hostname));
    API_COMPILE(WiFi.setHostname(shostname));
}
#endif // !HAL_PLATFORM_NCP
#endif // !HAL_PLATFORM_WIFI_SCAN_ONLY

#endif