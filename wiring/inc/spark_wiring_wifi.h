/**
 ******************************************************************************
 * @file    spark_wiring_wifi.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    7-Mar-2014
 * @brief   Header for spark_wiring_wifi.cpp module
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

#include "spark_wiring_platform.h"

#if Wiring_WiFi

#include "spark_wiring_network.h"
#include "wlan_hal.h"
#include "system_network.h"
#include "inet_hal.h"
#include "spark_wiring_wifi_credentials.h"
#include <string.h>
#include "spark_wiring_signal.h"

class WiFiSignal : public particle::Signal {
public:
    // In order to be compatible with CellularSignal
    int rssi = 2;
    int qual = 0;

    WiFiSignal() {}
    WiFiSignal(const wlan_connected_info_t& inf);
    virtual ~WiFiSignal() {};

    operator int8_t() const;

    bool fromConnectedInfo(const wlan_connected_info_t& sig);

    virtual hal_net_access_tech_t getAccessTechnology() const;
    virtual float getStrength() const;
    virtual float getStrengthValue() const;
    virtual float getQuality() const;
    virtual float getQualityValue() const;

    // virtual size_t printTo(Print& p) const;

private:
    wlan_connected_info_t inf_ = {0};
};

class IPAddress;

namespace spark {

class WiFiClass : public NetworkClass
{
    void setIPAddressSource(IPAddressSource source) {
        wlan_set_ipaddress_source(source, true, NULL);
    }

public:
    WiFiClass() {}
    ~WiFiClass() {}

    operator network_handle_t() {
        return 0;
    }

    WLanConfig* wifi_config() {
        return (WLanConfig*)network_config(*this, 0, NULL);
    }

    /**
     * Retrieves a 6-octet MAC address
     * @param mac
     * @return
     */
    uint8_t* macAddress(uint8_t *mac) {
        memcpy(mac, wifi_config()->nw.uaMacAddr, 6);
        return mac;
    }

    IPAddress localIP() {
        return IPAddress(wifi_config()->nw.aucIP);
    }

    IPAddress subnetMask() {
        return IPAddress(wifi_config()->nw.aucSubnetMask);
    }

    IPAddress gatewayIP() {
        return IPAddress(wifi_config()->nw.aucDefaultGateway);
    }

    IPAddress dnsServerIP() {
    	return IPAddress(wifi_config()->nw.aucDNSServer);
    }

    IPAddress dhcpServerIP() {
    	return IPAddress(wifi_config()->nw.aucDHCPServer);
    }

    uint8_t* BSSID(uint8_t* bssid) {
    		memcpy(bssid, wifi_config()->BSSID, 6);
    		return bssid;
    }

    const char *SSID() {
        return (const char *) wifi_config()->uaSSID;
    }

    WiFiSignal RSSI();
    uint32_t ping(IPAddress remoteIP) {
        return ping(remoteIP, 5);
    }

    uint32_t ping(IPAddress remoteIP, uint8_t nTries) {
        return inet_ping(&remoteIP.raw(), *this, nTries, NULL);
    }

    void connect(unsigned flags=0) {
        network_connect(*this, flags, 0, NULL);
    }

    void disconnect(void) {
        network_disconnect(*this, 0, NULL);
    }

    bool connecting(void) {
        return network_connecting(*this, 0, NULL);
    }

    bool ready(void) {
        return network_ready(*this, 0, NULL);
    }

    void on(void) {
        network_on(*this, 0, 0, NULL);
    }

    void off(void) {
        network_off(*this, 0, 0, NULL);
    }

    void listen(bool begin=true) {
        network_listen(*this, begin ? 0 : 1, NULL);
    }

    void setListenTimeout(uint16_t timeout) {
        network_set_listen_timeout(*this, timeout, NULL);
    }

    uint16_t getListenTimeout(void) {
        return network_get_listen_timeout(*this, 0, NULL);
    }

    bool listening(void) {
        return network_listening(*this, 0, NULL);
    }

    bool setCredentials(const char *ssid) {
        return setCredentials(ssid, NULL, UNSEC);
    }

    bool setCredentials(const char *ssid, const char *password) {
        return setCredentials(ssid, password, WPA2);
    }

    bool setCredentials(const char *ssid, const char *password, unsigned long security, unsigned long cipher=WLAN_CIPHER_NOT_SET) {
        return setCredentials(ssid, ssid ? strlen(ssid) : 0, password, password ? strlen(password) : 0, security, cipher);
    }

    bool setCredentials(const char *ssid, unsigned int ssidLen, const char *password,
            unsigned int passwordLen, unsigned long security=WLAN_SEC_UNSEC, unsigned long cipher=WLAN_CIPHER_NOT_SET) {

        WLanCredentials creds;
        memset(&creds, 0, sizeof(creds));
        creds.size = sizeof(creds);
        creds.ssid = ssid;
        creds.ssid_len = ssidLen;
        creds.password = password;
        creds.password_len = passwordLen;
        creds.security = WLanSecurityType(security);
        creds.cipher = WLanSecurityCipher(cipher);
        return (network_set_credentials(*this, 0, &creds, NULL) == 0);
    }

    void setCredentials(const char* ssid, WiFiCredentials credentials) {
        WLanCredentials creds = credentials.getHalCredentials();
        creds.ssid = ssid;
        creds.ssid_len = ssid ? strlen(ssid) : 0;
        network_set_credentials(*this, 0, &creds, NULL);
    }

    void setCredentials(WiFiCredentials credentials) {
        WLanCredentials creds = credentials.getHalCredentials();
        network_set_credentials(*this, 0, &creds, NULL);
    }

    bool hasCredentials(void) {
        return network_has_credentials(*this, 0, NULL);
    }

    bool clearCredentials(void) {
        return network_clear_credentials(*this, 0, NULL, NULL);
    }

    int selectAntenna(WLanSelectAntenna_TypeDef antenna) {
        return wlan_select_antenna(antenna);
    }

    IPAddress resolve(const char* name)
    {
        HAL_IPAddress ip;
        return (inet_gethostbyname(name, strlen(name), &ip, *this, NULL)<0) ?
                IPAddress(uint32_t(0)) : IPAddress(ip);
    }

    void setStaticIP(const IPAddress& host, const IPAddress& netmask,
        const IPAddress& gateway, const IPAddress& dns)
    {
        wlan_set_ipaddress(host, netmask, gateway, dns, NULL, NULL);
    }

    void useStaticIP()
    {
        setIPAddressSource(STATIC_IP);
    }

    void useDynamicIP()
    {
        setIPAddressSource(DYNAMIC_IP);
    }

    int scan(wlan_scan_result_t callback, void* cookie=NULL) {
        return wlan_scan(callback, cookie);
    }

    int scan(WiFiAccessPoint* results, size_t result_count);

    template <typename T>
    int scan(void (*handler)(WiFiAccessPoint* ap, T* instance), T* instance) {
        return scan((wlan_scan_result_t)handler, (void*)instance);
    }

    int getCredentials(WiFiAccessPoint* results, size_t result_count);

    String hostname()
    {
        const size_t maxHostname = 64;
        char buf[maxHostname] = {0};
        network_get_hostname(*this, 0, buf, maxHostname, nullptr);
        return String(buf);
    }

    int setHostname(const String& hostname)
    {
        return setHostname(hostname.c_str());
    }

    int setHostname(const char* hostname)
    {
        return network_set_hostname(*this, 0, hostname, nullptr);
    }
};

extern WiFiClass WiFi;

}   // namespace Spark

#endif // Wiring_WiFi

#endif
