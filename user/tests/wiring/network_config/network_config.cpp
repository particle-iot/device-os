/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "application.h"
#include "unit-test/unit-test.h"
#include "scope_guard.h"
#include "resolvapi.h"
#include "ifapi.h"

struct NetworkIp4Config {
    SockAddr addr;
    SockAddr netmask;
    uint8_t prefixLength;
    SockAddr gw;
    spark::Vector<SockAddr> dns;
    String profile;
};

NetworkIp4Config getNetworkAddress(network_interface_t idx) {
    if_t iface = nullptr;
    if (if_get_by_index(idx, &iface)) {
        return NetworkIp4Config();
    }
    if_addrs* ifAddrList = nullptr;
    if_get_addrs(iface, &ifAddrList);
    SCOPE_GUARD({
        if_free_if_addrs(ifAddrList);
    });
    NetworkIp4Config conf;
    for (if_addrs* i = ifAddrList; i; i = i->next) {
        if (!i->if_addr) {
            continue;
        }
        // Only checking AF_INET addresses as of right now
        if (i->if_addr->addr->sa_family != AF_INET) {
            continue;
        }
        SockAddr addr(i->if_addr->addr);
        SockAddr mask(i->if_addr->netmask);
        SockAddr gw(i->if_addr->gw);
        conf.addr = addr;
        conf.netmask = mask;
        conf.prefixLength = i->if_addr->prefixlen;
        conf.gw = gw;
        return conf;
    }
    return NetworkIp4Config();
}

spark::Vector<SockAddr> getDnsServerList() {
    spark::Vector<SockAddr> dnsList;
    resolv_dns_servers* dns = {};
    resolv_get_dns_servers(&dns);
    SCOPE_GUARD({
        resolv_free_dns_servers(dns);
    });
    // Take only the first one
    for (auto s = dns; s != nullptr; s = s->next) {
        // Only IPv4 for now
        SockAddr a(s->server);
        if (a.family() == AF_INET) {
            dnsList.append(a);
        }
    }
    return dnsList;
}

bool networkInterfaceConfigMatches(const NetworkInterfaceConfig& lhs, const NetworkInterfaceConfig& rhs) {
    // AF_INET for now only
    if (lhs.source(AF_INET) != rhs.source(AF_INET)) {
        return false;
    }
    if (lhs.addresses(AF_INET) != rhs.addresses(AF_INET)) {
        return false;
    }
    if (lhs.dns(AF_INET) != rhs.dns(AF_INET)) {
        return false;
    }
    if (lhs.profile() != rhs.profile()) {
        return false;
    }

    return true;
}

test(NETWORK_CONFIG_00_prepare) {
    // Make sure we start with a clean slate
    unlink("/sys/network.dat");
}

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY

test(NETWORK_CONFIG_WIFI_00_prepare) {
    Particle.disconnect();
#if HAL_PLATFORM_WIFI
    WiFi.disconnect();
#endif // HAL_PLATFORM_WIFI
#if HAL_PLATFORM_CELLULAR
    Cellular.disconnect();
#endif // HAL_PLATFORM_CELLULAR
#if HAL_PLATFORM_ETHERNET
    Ethernet.disconnect();
#endif // HAL_PLATFORM_ETHERNET
}

namespace {

bool wifiConfigOk = false;
NetworkIp4Config wifiConfig;

}

test(NETWORK_CONFIG_WIFI_01_query_expected_settings) {
    WiFi.connect();
    assertTrue(waitFor(WiFi.ready, 60000));
    auto addr = getNetworkAddress(WiFi);
    assertFalse(addr.addr.isAddrAny());
    assertFalse(addr.netmask.isAddrAny());
    assertFalse(addr.gw.isAddrAny());
    assertEqual(addr.addr.family(), AF_INET);
    assertEqual(addr.netmask.family(), AF_INET);
    assertEqual(addr.gw.family(), AF_INET);
    assertNotEqual(addr.prefixLength, 0);
    assertNotEqual(addr.prefixLength, 32);

    auto dns = getDnsServerList();
    assertNotEqual(dns.size(), 0);
    addr.dns = dns;

    const auto ssid = WiFi.SSID();
    assertFalse(ssid == nullptr);
    assertNotEqual(strlen(ssid), 0);
    addr.profile = String(ssid);

    wifiConfigOk = true;
    wifiConfig = addr;
}

test(NETWORK_CONFIG_WIFI_02_disconnect) {
    assertTrue(wifiConfigOk);

    WiFi.disconnect();
}

test(NETWORK_CONFIG_WIFI_03_static_config_matching_dhcp) {
    assertTrue(wifiConfigOk);

    auto conf = NetworkInterfaceConfig()
            .profile(wifiConfig.profile)
            .source(NetworkInterfaceConfigSource::STATIC)
            .address(wifiConfig.addr, wifiConfig.netmask)
            .gateway(wifiConfig.gw);
    // Apply DNS in reverse order
    for (int i = wifiConfig.dns.size() - 1; i >= 0; i--) {
        conf.dns(wifiConfig.dns[i]);
    }
    assertEqual(0, WiFi.setConfig(conf));
    auto storedConf = WiFi.getConfig(wifiConfig.profile);
    assertTrue(networkInterfaceConfigMatches(conf, storedConf));

    WiFi.connect();
    assertTrue(waitFor(WiFi.ready, 60000));
    Particle.connect();
    assertTrue(waitFor(Particle.connected, 60000));

    assertFalse((bool)WiFi.dhcpServerIP());
    
    auto addr = getNetworkAddress(WiFi);
    assertFalse(addr.addr.isAddrAny());
    assertFalse(addr.netmask.isAddrAny());
    assertFalse(addr.gw.isAddrAny());
    assertEqual(addr.addr.family(), AF_INET);
    assertEqual(addr.netmask.family(), AF_INET);
    assertEqual(addr.gw.family(), AF_INET);
    assertNotEqual(addr.prefixLength, 0);
    assertNotEqual(addr.prefixLength, 32);

    auto dns = getDnsServerList();
    assertNotEqual(dns.size(), 0);
    addr.dns = dns;

    const auto ssid = WiFi.SSID();
    assertFalse(ssid == nullptr);
    assertNotEqual(strlen(ssid), 0);
    addr.profile = String(ssid);

    assertTrue(addr.addr == wifiConfig.addr);
    assertTrue(addr.netmask == wifiConfig.netmask);
    assertTrue(addr.gw == wifiConfig.gw);
    assertTrue(addr.prefixLength == wifiConfig.prefixLength);
    assertEqual(addr.dns.size(), wifiConfig.dns.size());
    for (int i = 0; i < addr.dns.size(); i++) {
        assertTrue(addr.dns[i] == wifiConfig.dns[wifiConfig.dns.size() - i - 1]);
    }
    assertEqual(addr.profile, wifiConfig.profile);
}

test(NETWORK_CONFIG_WIFI_04_disconnect) {
    assertTrue(wifiConfigOk);

    Particle.disconnect();
    WiFi.disconnect();
}

test(NETWORK_CONFIG_WIFI_05_dhcp_with_gw_and_dns_override) {
    assertTrue(wifiConfigOk);

    auto conf = NetworkInterfaceConfig()
            .profile(wifiConfig.profile)
            .source(NetworkInterfaceConfigSource::DHCP)
            .dns(SockAddr("8.8.8.8"))
            .dns(SockAddr("1.1.1.1"))
            .gateway(SockAddr("1.2.3.4"));
    assertEqual(0, WiFi.setConfig(conf));
    auto storedConf = WiFi.getConfig(wifiConfig.profile);
    assertTrue(networkInterfaceConfigMatches(conf, storedConf));

    WiFi.connect();
    assertTrue(waitFor(WiFi.ready, 60000));

    assertTrue((bool)WiFi.dhcpServerIP());
    
    auto addr = getNetworkAddress(WiFi);
    assertFalse(addr.addr.isAddrAny());
    assertFalse(addr.netmask.isAddrAny());
    assertFalse(addr.gw.isAddrAny());
    assertEqual(addr.addr.family(), AF_INET);
    assertEqual(addr.netmask.family(), AF_INET);
    assertEqual(addr.gw.family(), AF_INET);
    assertNotEqual(addr.prefixLength, 0);
    assertNotEqual(addr.prefixLength, 32);

    auto dns = getDnsServerList();
    assertNotEqual(dns.size(), 0);
    addr.dns = dns;

    const auto ssid = WiFi.SSID();
    assertFalse(ssid == nullptr);
    assertNotEqual(strlen(ssid), 0);
    addr.profile = String(ssid);

    assertTrue(addr.netmask == wifiConfig.netmask);
    assertTrue(addr.gw == SockAddr("1.2.3.4"));
    assertTrue(addr.prefixLength == wifiConfig.prefixLength);
    assertEqual(addr.dns.size(), 2);
    assertTrue(addr.dns[0] == SockAddr("8.8.8.8"));
    assertTrue(addr.dns[1] == SockAddr("1.1.1.1"));
    assertEqual(addr.profile, wifiConfig.profile);
}

test(NETWORK_CONFIG_WIFI_06_disconnect) {
    assertTrue(wifiConfigOk);

    Particle.disconnect();
    WiFi.disconnect();
}

test(NETWORK_CONFIG_WIFI_07_dhcp_with_no_gw) {
    assertTrue(wifiConfigOk);

    auto conf = NetworkInterfaceConfig()
            .profile(wifiConfig.profile)
            .source(NetworkInterfaceConfigSource::DHCP)
            .dns(SockAddr("8.8.8.8"))
            .dns(SockAddr("1.1.1.1"))
            .gateway(SockAddr("0.0.0.0"));
    assertEqual(0, WiFi.setConfig(conf));
    auto storedConf = WiFi.getConfig(wifiConfig.profile);
    assertTrue(networkInterfaceConfigMatches(conf, storedConf));

    WiFi.connect();
    // WiFi.ready() is not going to become true because gateway is going to be 0.0.0.0
    assertTrue(waitFor(WiFi.dhcpServerIP, 60000));
    
    auto addr = getNetworkAddress(WiFi);
    assertFalse(addr.addr.isAddrAny());
    assertFalse(addr.netmask.isAddrAny());
    assertTrue(addr.gw.isAddrAny());
    assertEqual(addr.addr.family(), AF_INET);
    assertEqual(addr.netmask.family(), AF_INET);
    assertEqual(addr.gw.family(), AF_INET);
    assertNotEqual(addr.prefixLength, 0);
    assertNotEqual(addr.prefixLength, 32);

    auto dns = getDnsServerList();
    assertNotEqual(dns.size(), 0);
    addr.dns = dns;

    const auto ssid = WiFi.SSID();
    assertFalse(ssid == nullptr);
    assertNotEqual(strlen(ssid), 0);
    addr.profile = String(ssid);

    assertTrue(addr.netmask == wifiConfig.netmask);
    assertTrue(addr.gw == SockAddr("0.0.0.0"));
    assertTrue(addr.prefixLength == wifiConfig.prefixLength);
    assertEqual(addr.dns.size(), 2);
    assertTrue(addr.dns[0] == SockAddr("8.8.8.8"));
    assertTrue(addr.dns[1] == SockAddr("1.1.1.1"));
    assertEqual(addr.profile, wifiConfig.profile);
}

test(NETWORK_CONFIG_WIFI_98_restore) {
    Particle.disconnect();
    WiFi.disconnect();

    auto conf = NetworkInterfaceConfig()
            .profile(wifiConfig.profile);
    assertEqual(0, WiFi.setConfig(conf));
    auto storedConf = WiFi.getConfig(wifiConfig.profile);
    assertTrue(networkInterfaceConfigMatches(conf, storedConf));
}

test(NETWORK_CONFIG_WIFI_99_connect) {
    WiFi.connect();
    assertTrue(waitFor(WiFi.ready, 60000));
    assertTrue((bool)WiFi.dhcpServerIP());
    Particle.connect();
    assertTrue(waitFor(Particle.connected, 60000));
}

#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY

#if HAL_PLATFORM_ETHERNET

static bool isEthernetPresent() {
    if_t iface = nullptr;
    return !if_get_by_index(NETWORK_INTERFACE_ETHERNET, &iface);
}

test(NETWORK_CONFIG_ETH_01_enable_feature) {
    System.enableFeature(FEATURE_ETHERNET_DETECTION);
    // Notify about a pending reset
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 5000));
    System.reset();
}

test(NETWORK_CONFIG_ETH_02_check_eth) {
    if (!isEthernetPresent()) {
#ifndef REQUIRE_ETHERNET
        skip();
#else
        assertTrue(isEthernetPresent());
#endif // REQUIRE_ETHERNET
        return;
    }
}

test(NETWORK_CONFIG_ETH_03_prepare) {
    if (!isEthernetPresent()) {
        skip();
        return;
    }

    Particle.disconnect();
#if HAL_PLATFORM_WIFI
    WiFi.disconnect();
#endif // HAL_PLATFORM_WIFI
#if HAL_PLATFORM_CELLULAR
    Cellular.disconnect();
#endif // HAL_PLATFORM_CELLULAR
    Ethernet.disconnect();
}

namespace {

bool ethConfigOk = false;
NetworkIp4Config ethConfig;

}

test(NETWORK_CONFIG_ETH_04_query_expected_settings) {
    if (!isEthernetPresent()) {
        skip();
        return;
    }

    Ethernet.connect();
    assertTrue(waitFor(Ethernet.ready, 60000));
    auto addr = getNetworkAddress(Ethernet);
    assertFalse(addr.addr.isAddrAny());
    assertFalse(addr.netmask.isAddrAny());
    assertFalse(addr.gw.isAddrAny());
    assertEqual(addr.addr.family(), AF_INET);
    assertEqual(addr.netmask.family(), AF_INET);
    assertEqual(addr.gw.family(), AF_INET);
    assertNotEqual(addr.prefixLength, 0);
    assertNotEqual(addr.prefixLength, 32);

    auto dns = getDnsServerList();
    assertNotEqual(dns.size(), 0);
    addr.dns = dns;

    ethConfigOk = true;
    ethConfig = addr;
}

test(NETWORK_CONFIG_ETH_05_disconnect) {
    if (!isEthernetPresent()) {
        skip();
        return;
    }

    assertTrue(ethConfigOk);

    Ethernet.disconnect();
}

test(NETWORK_CONFIG_ETH_06_static_config_matching_dhcp) {
    if (!isEthernetPresent()) {
        skip();
        return;
    }

    assertTrue(ethConfigOk);

    auto conf = NetworkInterfaceConfig()
            .source(NetworkInterfaceConfigSource::STATIC)
            .address(ethConfig.addr, ethConfig.netmask)
            .gateway(ethConfig.gw);
    // Apply DNS in reverse order
    for (int i = ethConfig.dns.size() - 1; i >= 0; i--) {
        conf.dns(ethConfig.dns[i]);
    }
    assertEqual(0, Ethernet.setConfig(conf));
    auto storedConf = Ethernet.getConfig(ethConfig.profile);
    assertTrue(networkInterfaceConfigMatches(conf, storedConf));

    Ethernet.connect();
    assertTrue(waitFor(Ethernet.ready, 60000));
    Particle.connect();
    assertTrue(waitFor(Particle.connected, 60000));

    // FIXME
    // assertFalse((bool)Ethernet.dhcpServerIP());
    
    auto addr = getNetworkAddress(Ethernet);
    assertFalse(addr.addr.isAddrAny());
    assertFalse(addr.netmask.isAddrAny());
    assertFalse(addr.gw.isAddrAny());
    assertEqual(addr.addr.family(), AF_INET);
    assertEqual(addr.netmask.family(), AF_INET);
    assertEqual(addr.gw.family(), AF_INET);
    assertNotEqual(addr.prefixLength, 0);
    assertNotEqual(addr.prefixLength, 32);

    auto dns = getDnsServerList();
    assertNotEqual(dns.size(), 0);
    addr.dns = dns;

    assertTrue(addr.addr == ethConfig.addr);
    assertTrue(addr.netmask == ethConfig.netmask);
    assertTrue(addr.gw == ethConfig.gw);
    assertTrue(addr.prefixLength == ethConfig.prefixLength);
    assertEqual(addr.dns.size(), ethConfig.dns.size());
    for (int i = 0; i < addr.dns.size(); i++) {
        assertTrue(addr.dns[i] == ethConfig.dns[ethConfig.dns.size() - i - 1]);
    }
    assertEqual(addr.profile, ethConfig.profile);
}

test(NETWORK_CONFIG_ETH_07_disconnect) {
    if (!isEthernetPresent()) {
        skip();
        return;
    }

    assertTrue(ethConfigOk);

    Particle.disconnect();
    Ethernet.disconnect();
}

test(NETWORK_CONFIG_ETH_08_dhcp_with_gw_and_dns_override) {
    if (!isEthernetPresent()) {
        skip();
        return;
    }

    assertTrue(ethConfigOk);

    auto conf = NetworkInterfaceConfig()
            .source(NetworkInterfaceConfigSource::DHCP)
            .dns(SockAddr("8.8.8.8"))
            .dns(SockAddr("1.1.1.1"))
            .gateway(SockAddr("1.2.3.4"));
    assertEqual(0, Ethernet.setConfig(conf));
    auto storedConf = Ethernet.getConfig(ethConfig.profile);
    assertTrue(networkInterfaceConfigMatches(conf, storedConf));

    Ethernet.connect();
    assertTrue(waitFor(Ethernet.ready, 60000));

    // FIXME
    // assertTrue((bool)Ethernet.dhcpServerIP());
    
    auto addr = getNetworkAddress(Ethernet);
    assertFalse(addr.addr.isAddrAny());
    assertFalse(addr.netmask.isAddrAny());
    assertFalse(addr.gw.isAddrAny());
    assertEqual(addr.addr.family(), AF_INET);
    assertEqual(addr.netmask.family(), AF_INET);
    assertEqual(addr.gw.family(), AF_INET);
    assertNotEqual(addr.prefixLength, 0);
    assertNotEqual(addr.prefixLength, 32);

    auto dns = getDnsServerList();
    assertNotEqual(dns.size(), 0);
    addr.dns = dns;

    assertTrue(addr.netmask == ethConfig.netmask);
    assertTrue(addr.gw == SockAddr("1.2.3.4"));
    assertTrue(addr.prefixLength == ethConfig.prefixLength);
    assertEqual(addr.dns.size(), 2);
    assertTrue(addr.dns[0] == SockAddr("8.8.8.8"));
    assertTrue(addr.dns[1] == SockAddr("1.1.1.1"));
    assertEqual(addr.profile, ethConfig.profile);
}

test(NETWORK_CONFIG_ETH_08_disconnect) {
    if (!isEthernetPresent()) {
        skip();
        return;
    }

    assertTrue(ethConfigOk);

    Particle.disconnect();
    Ethernet.disconnect();
}

test(NETWORK_CONFIG_ETH_09_dhcp_with_no_gw) {
    if (!isEthernetPresent()) {
        skip();
        return;
    }

    assertTrue(ethConfigOk);

    auto conf = NetworkInterfaceConfig()
            .source(NetworkInterfaceConfigSource::DHCP)
            .dns(SockAddr("8.8.8.8"))
            .dns(SockAddr("1.1.1.1"))
            .gateway(SockAddr("0.0.0.0"));
    assertEqual(0, Ethernet.setConfig(conf));
    auto storedConf = Ethernet.getConfig(ethConfig.profile);
    assertTrue(networkInterfaceConfigMatches(conf, storedConf));

    Ethernet.connect();
    // Ethernet.ready() is not going to become true because gateway is going to be 0.0.0.0
    assertTrue(waitFor(Ethernet.localIP, 60000));
    
    auto addr = getNetworkAddress(Ethernet);
    assertFalse(addr.addr.isAddrAny());
    assertFalse(addr.netmask.isAddrAny());
    assertTrue(addr.gw.isAddrAny());
    assertEqual(addr.addr.family(), AF_INET);
    assertEqual(addr.netmask.family(), AF_INET);
    assertEqual(addr.gw.family(), AF_INET);
    assertNotEqual(addr.prefixLength, 0);
    assertNotEqual(addr.prefixLength, 32);

    auto dns = getDnsServerList();
    assertNotEqual(dns.size(), 0);
    addr.dns = dns;

    assertTrue(addr.netmask == ethConfig.netmask);
    assertTrue(addr.gw == SockAddr("0.0.0.0"));
    assertTrue(addr.prefixLength == ethConfig.prefixLength);
    assertEqual(addr.dns.size(), 2);
    assertTrue(addr.dns[0] == SockAddr("8.8.8.8"));
    assertTrue(addr.dns[1] == SockAddr("1.1.1.1"));
    assertEqual(addr.profile, ethConfig.profile);
}

test(NETWORK_CONFIG_ETH_97_restore) {
    if (!isEthernetPresent()) {
        skip();
        return;
    }

    Particle.disconnect();
    Ethernet.disconnect();

    auto conf = NetworkInterfaceConfig();
    assertEqual(0, Ethernet.setConfig(conf));
    auto storedConf = Ethernet.getConfig(ethConfig.profile);
    assertTrue(networkInterfaceConfigMatches(conf, storedConf));
}

test(NETWORK_CONFIG_ETH_98_connect) {
    if (!isEthernetPresent()) {
        skip();
        return;
    }

    Ethernet.connect();
    assertTrue(waitFor(Ethernet.ready, 60000));
    assertTrue((bool)Ethernet.localIP());
    Particle.connect();
    assertTrue(waitFor(Particle.connected, 60000));
}

test(NETWORK_CONFIG_ETH_99_disable_feature) {
    System.disableFeature(FEATURE_ETHERNET_DETECTION);
    // Notify about a pending reset
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 5000));
    System.reset();
}
#endif // HAL_PLATFORM_ETHERNET

test(NETWORK_CONFIG_zz_reset) {
    // Make sure we clean up configuration entirely
    unlink("/sys/network.dat");
}
