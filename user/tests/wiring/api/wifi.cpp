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
	uint8_t* ether = nullptr;
	String ssid;
	API_COMPILE(ssid=WiFi.SSID());
	API_COMPILE(address=WiFi.localIP());
	API_COMPILE(address=WiFi.dnsServerIP());
	API_COMPILE(address=WiFi.dhcpServerIP());
	API_COMPILE(address=WiFi.gatewayIP());
	API_COMPILE(ether=WiFi.macAddress(ether));
	API_COMPILE(ether=WiFi.BSSID(ether));
}

test(api_wifi_resolve)
{
    API_COMPILE(WiFi.resolve(String("abc.def.com")));
    API_COMPILE(WiFi.resolve("abc.def.com"));
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
    API_COMPILE(val = WiFi.getListenTimeout());
    (void)result; // avoid unused warning
    (void)val;    //   |
}

#if PLATFORM_ID>=4
test(api_wifi_selectantenna)
{
    API_COMPILE(WiFi.selectAntenna(ANT_AUTO));
    API_COMPILE(WiFi.selectAntenna(ANT_INTERNAL));
    API_COMPILE(WiFi.selectAntenna(ANT_EXTERNAL));
}
#endif


test(api_wifi_set_credentials)
{
    bool ok = false;
    API_COMPILE(ok = WiFi.setCredentials("ssid)",4,"password", 8, WPA2));
    API_COMPILE(ok = WiFi.setCredentials("ssid)",4,"password", 8, WPA2, WLAN_CIPHER_AES));
    API_COMPILE(ok = WiFi.setCredentials("ssid)","password", WPA2, WLAN_CIPHER_AES));
    (void)ok; // avoid unused warning
}

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

test(api_wifi_hostname)
{
    String hostname;
    const char shostname[] = "testhostname";
    API_COMPILE(hostname = WiFi.hostname());
    API_COMPILE(WiFi.setHostname(hostname));
    API_COMPILE(WiFi.setHostname(shostname));
}

#endif
