/**
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

#include "application.h"
#include "unit-test/unit-test.h"
#include "scope_guard.h"

#if Wiring_WiFi == 1

test(WIFI_01_resolve_3_levels)
{
    IPAddress address = WiFi.resolve("pool.ntp.org");
    assertNotEqual(address, 0);

    // ensure the version field is set
    assertNotEqual(address.version(), 0);

    IPAddress compare = IPAddress(address[0], address[1], address[2], address[3]);
    assertTrue(compare==address);
}

test(WIFI_02_resolve_4_levels)
{
    IPAddress address = WiFi.resolve("north-america.pool.ntp.org");
    assertNotEqual(address, 0);
}

test(WIFI_03_resolve) {
    IPAddress addr = WiFi.resolve("this.is.not.a.real.host");
    assertEqual(addr, 0);
}

void checkIPAddress(const char* name, const IPAddress& address)
{
    if (address.version()==0 || !address)
    {
        Serial.print("address failed:");
        Serial.println(name);
        assertNotEqual(address.version(), 0);
        assertNotEqual(address, 0);
    }
}

void checkEtherAddress(const uint8_t* address)
{
    uint8_t sum = 0;
    for (int i=0; i<6; i++)
    {
        sum |= address[i];
    }
    assertNotEqual(sum, 0);
}

test(WIFI_04_config)
{
    checkIPAddress("local", WiFi.localIP());
    checkIPAddress("gateway", WiFi.gatewayIP());

    uint8_t ether[6];
    uint8_t ether2[6];
    memset(ether, 0, 6);
    memset(ether2, 0, 6);

    assertTrue(WiFi.macAddress(ether)==ether);
    assertTrue(WiFi.macAddress(ether2)==ether2);
    checkEtherAddress(ether);
    assertTrue(!memcmp(ether, ether2, 6))

    memset(ether, 0, 6);
    memset(ether2, 0, 6);
    assertTrue(WiFi.BSSID(ether)==ether);
    assertTrue(WiFi.BSSID(ether2)==ether2);
    checkEtherAddress(ether);
    assertTrue(!memcmp(ether, ether2, 6));

#if !HAL_PLATFORM_NCP
    // FIXME: this is not available on Gen 3 devices yet
    checkIPAddress("dnsServer", WiFi.dnsServerIP());
    checkIPAddress("dhcpServer", WiFi.dhcpServerIP());
#endif // !HAL_PLATFORM_NCP
}

test(WIFI_05_scan)
{
    spark::Vector<WiFiAccessPoint> aps(20);
    int apsFound = WiFi.scan(aps.data(), 20);
    assertMoreOrEqual(apsFound, 1);
}

#if PLATFORM_ID == 6 || PLATFORM_ID == 8

test(WIFI_06_reconnections_that_use_wlan_restart_dont_cause_memory_leaks)
{
    /* This test should only be run with threading disabled */
    if (system_thread_get_state(nullptr) == spark::feature::ENABLED) {
        skip();
        return;
    }

    assertTrue(Particle.connected());

    Particle.disconnect();
    waitFor(Particle.disconnected, 10000);
    assertTrue(Particle.disconnected());

    WiFi.disconnect();
    uint32_t ms = millis();
    while (WiFi.ready()) {
        if (millis() - ms >= 10000) {
            assertTrue(false);
        }
    }

    set_system_mode(SEMI_AUTOMATIC);

    Particle.connect();
    waitFor(Particle.connected, 10000);

    Particle.disconnect();
    waitFor(Particle.disconnected, 10000);
    assertTrue(Particle.disconnected());

    WiFi.disconnect();
    ms = millis();
    while (WiFi.ready()) {
        if (millis() - ms >= 10000) {
            assertTrue(false);
        }
    }

    uint32_t freeRam1 = System.freeMemory();

    wlan_restart(NULL);

    Particle.connect();
    waitFor(Particle.connected, 10000);

    Particle.disconnect();
    waitFor(Particle.disconnected, 10000);
    assertTrue(Particle.disconnected());

    WiFi.disconnect();
    ms = millis();
    while (WiFi.ready()) {
        if (millis() - ms >= 10000) {
            assertTrue(false);
        }
    }

    uint32_t freeRam2 = System.freeMemory();

    assertMoreOrEqual(freeRam2, freeRam1);
}

#endif // PLATFORM_ID == 6 || PLATFORM_ID == 8

test(WIFI_07_restore_connection)
{
    set_system_mode(AUTOMATIC);
    if (!Particle.connected())
    {
        Particle.connect();
    }
}

#if PLATFORM_ID == 6 || PLATFORM_ID == 8

#if !HAL_PLATFORM_NCP
test(WIFI_08_reset_hostname)
{
    assertEqual(WiFi.setHostname(NULL), 0);
}

test(WIFI_09_default_hostname_equals_device_id)
{
    String hostname = WiFi.hostname();
    String devId = System.deviceID();
    assertEqual(hostname, devId);
}

test(WIFI_10_custom_hostname_can_be_set)
{
    String hostname("testhostname");
    assertEqual(WiFi.setHostname(hostname), 0);
    assertEqual(WiFi.hostname(), hostname);
}

test(WIFI_11_restore_default_hostname)
{
    assertEqual(WiFi.setHostname(NULL), 0);
}
#endif //!HAL_PLATFORM_NCP

#endif // PLATFORM_ID == 6 || PLATFORM_ID == 8

test(WIFI_12_scan_returns_zero_result_or_error_when_wifi_is_off)
{
    WiFiAccessPoint results[5];
    WiFi.off();
    uint32_t ms = millis();
    while (WiFi.ready()) {
        if (millis() - ms >= 10000) {
            assertTrue(false);
        }
    }
    // Delay a bit here just in case
    delay(5000);
    assertLessOrEqual(WiFi.scan(results, 5), 0);
}

test(WIFI_13_restore_connection)
{
    if (!Particle.connected())
    {
        Particle.connect();
    }
}

test(WIFI_14_wifi_class_methods_work_correctly_when_wifi_interface_is_off) {
    Particle.disconnect();
    WiFi.disconnect();
    WiFi.off();
    delay(1000);
    SCOPE_GUARD({
        // Restore the connection
        WiFi.on();
        WiFi.connect();
        Particle.connect();
    });
    waitForNot(WiFi.ready, 30000);
    assertEqual(WiFi.ready(), false);

    uint8_t tmp[6];

    // These methods should still work correctly while the WiFi interface
    // is turned off

    // Don't care about the result. Some platforms
    // might be able to provide the MAC with WiFi interface off,
    // some might not.
    (void)WiFi.macAddress(tmp);

    // The rest of the functions use this method and some of them
    // don't have nullptr checks.
    auto conf = WiFi.wifi_config();
    assertFalse(conf == nullptr);

    // All these should be 0.0.0.0
    IPAddress zeroAddr;
    assertEqual(WiFi.localIP(), zeroAddr);
    assertEqual(WiFi.subnetMask(), zeroAddr);
    assertEqual(WiFi.gatewayIP(), zeroAddr);
    assertEqual(WiFi.dnsServerIP(), zeroAddr);
    assertEqual(WiFi.dhcpServerIP(), zeroAddr);
    auto sig = WiFi.RSSI();
    assertEqual(sig.getQuality(), -1.0f);
    assertEqual(sig.getQualityValue(), 0.0f);
    assertEqual(sig.getStrength(), -1.0f);
    assertEqual(sig.getStrengthValue(), 0.0f);
    assertNotEqual(WiFi.getAntenna(), ANT_NONE);
    auto ssid = WiFi.SSID();
    if (ssid) {
        assertEqual(strlen(WiFi.SSID()), 0);
    }
    // 00:00:00:00:00:00 or ff:ff:ff:ff:ff:ff
    auto bssid = WiFi.BSSID(tmp);
    const uint8_t bssidRef[6] = {};
    const uint8_t bssidRefFf[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    assertTrue(!memcmp(bssidRef, bssid, sizeof(bssidRef)) || !memcmp(bssidRefFf, bssid, sizeof(bssidRefFf)));
}

#endif
