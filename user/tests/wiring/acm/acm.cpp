/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

const uint32_t WIFI_CONNECT_TIMEOUT_MS = 5 * 60 * 1000;
const uint32_t DISCONNECT_TIMEOUT_MS = 5 * 60 * 1000;

unsigned s_availableNetworkCount = 0;
bool s_ethernetAvailable = false;
bool s_skipNonEthernet = false;

void preferConnectTest(NetworkClass& network, bool& ok) {
    ok = false;
    Particle.disconnect();
    Network.disconnect();
    assertTrue(waitForNot(Particle.connected, DISCONNECT_TIMEOUT_MS));
    delay(1000);

    network.prefer();
    Network.connect();
    assertTrue(System.waitCondition([&network](){ return network.ready(); }, WIFI_CONNECT_TIMEOUT_MS));
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertEqual(network, Particle.connectionInterface());
    ok = true;
}

void preferSwitchTest(NetworkClass& network, bool connect, bool& ok) {
    ok = false;
    Particle.disconnect();
    Network.disconnect();
    assertTrue(waitForNot(Particle.connected, DISCONNECT_TIMEOUT_MS));
    delay(5000);

    // Default preference
    Network.prefer();
    // FIXME: why is this required? Might have a problem with network state machine
    Network.connect();
    // Connect all interfaces just in case
#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
    if (network != WiFi || connect) {
        WiFi.on();
        WiFi.connect();
    }
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
#if HAL_PLATFORM_CELLULAR
    if (network != Cellular || connect) {
        Cellular.on();
        Cellular.connect();
    }
#endif // HAL_PLATFORM_CELLULAR
#if HAL_PLATFORM_ETHERNET
    if (network != Ethernet || connect) {
        Ethernet.on();
        Ethernet.connect();
    }
#endif // HAL_PLATFORM_ETHERNET
    if (!connect) {
        network.disconnect();
        // Just in case
        network.off();
        assertTrue(System.waitCondition([&network](){ return !network.isOn(); }, DISCONNECT_TIMEOUT_MS));
    }
#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
    if (network != WiFi || connect) {
        assertTrue(waitFor(WiFi.ready, WIFI_CONNECT_TIMEOUT_MS));
    }
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
#if HAL_PLATFORM_CELLULAR
    if (network != Cellular || connect) {
        assertTrue(waitFor(Cellular.ready, WIFI_CONNECT_TIMEOUT_MS));
    }
#endif // HAL_PLATFORM_CELLULAR
#if HAL_PLATFORM_ETHERNET
    if (network != Ethernet || connect) {
        if (s_ethernetAvailable) {
            assertTrue(waitFor(Ethernet.ready, WIFI_CONNECT_TIMEOUT_MS));
        }
    }
#endif // HAL_PLATFORM_ETHERNET
    Particle.connect();
    // May switchover to a better candidate after a connection is established
    delay(10000);
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    auto connectedNetwork = Particle.connectionInterface();
    bool switched = false;
    // Prefer whatever network we did not connect on in order to force the cloud connection to switch
    if (connectedNetwork == network) {
#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
        if (network != WiFi) {
            assertTrue(waitFor(WiFi.ready, WIFI_CONNECT_TIMEOUT_MS));
            WiFi.prefer();
            assertTrue(WiFi.isPreferred());
            switched = true;
        } else
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
#if HAL_PLATFORM_CELLULAR
        if (network != Cellular) {
            assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_CELLULAR_CONN_TIMEOUT));
            Cellular.prefer();
            assertTrue(Cellular.isPreferred());
            switched = true;
        } else
#endif // HAL_PLATFORM_CELLULAR
#if HAL_PLATFORM_ETHERNET
        if (network != Ethernet && s_ethernetAvailable) {
            assertTrue(waitFor(Ethernet.ready, WIFI_CONNECT_TIMEOUT_MS));
            Ethernet.prefer();
            assertTrue(Ethernet.isPreferred());
            switched = true;
        }
#endif // HAL_PLATFORM_ETHERNET
    }
    if (switched) {
        // Verify that we disconnect and reconnect on the newly preferred network
        for (int i = 0; i < 2; i++) {
            assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
            if (connectedNetwork != Particle.connectionInterface()) {
                break;
            }
            delay(5000);
        }
        assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
        assertNotEqual(connectedNetwork, Particle.connectionInterface());
    }
    network.prefer();
    if (!connect) {
        network.connect();
    }
    assertTrue(network.isPreferred());
    assertTrue(System.waitCondition([&network](){ return network.ready(); }, WIFI_CONNECT_TIMEOUT_MS));
    for (int i = 0; i < 2; i++) {
        assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
        if (network == Particle.connectionInterface()) {
            break;
        }
        delay(5000);
    }
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertEqual(network, Particle.connectionInterface());
#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
    if (network != WiFi) {
        assertFalse(WiFi.isPreferred());
    }
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
#if HAL_PLATFORM_CELLULAR
    if (network != Cellular) {
        assertFalse(Cellular.isPreferred());
    }
#endif // HAL_PLATFORM_CELLULAR
#if HAL_PLATFORM_ETHERNET
    if (network != Ethernet) {
        assertFalse(Ethernet.isPreferred());
    }
#endif // HAL_PLATFORM_ETHERNET
    ok = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

#if HAL_PLATFORM_ETHERNET
test(ACM_00_prepare_ethernet) {
    System.enableFeature(FEATURE_ETHERNET_DETECTION);
    // Notify about a pending reset
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 5000));
    System.reset();
}
#endif // HAL_PLATFORM_ETHERNET

test(ACM_01_prepare) {
    Particle.disconnect();

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
    WiFi.on();
    WiFi.disconnect();
    s_availableNetworkCount++;
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
#if HAL_PLATFORM_CELLULAR
    Cellular.on();
    Cellular.disconnect();
    s_availableNetworkCount++;
#endif // HAL_PLATFORM_CELLULAR
#if HAL_PLATFORM_ETHERNET
    Ethernet.on();
    Ethernet.disconnect();
    if (waitFor(Ethernet.isOn, 5000)) {
        s_availableNetworkCount++;
        s_ethernetAvailable = true;
    }
#endif // HAL_PLATFORM_ETHERNET

    assertEqual(Network, Particle.connectionInterface());
}

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
test(ACM_02_particle_connect_uses_preferred_wifi_network) {
    assertMoreOrEqual(s_availableNetworkCount, 2);
    bool ok = false;
    preferConnectTest(WiFi, ok);
    assertTrue(ok);
}
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY

#if HAL_PLATFORM_CELLULAR
test(ACM_03_particle_connect_uses_preferred_cellular_network) {
    assertMoreOrEqual(s_availableNetworkCount, 2);
    bool ok = false;
    preferConnectTest(Cellular, ok);
    assertTrue(ok);
}
#endif // HAL_PLATFORM_CELLULAR

#if HAL_PLATFORM_ETHERNET
test(ACM_04_particle_connect_uses_preferred_ethernet_network) {
    assertMoreOrEqual(s_availableNetworkCount, 2);
    if (!s_ethernetAvailable) {
        skip();
        return;
    }
    bool ok = false;
    preferConnectTest(Ethernet, ok);
    assertTrue(ok);
}
#endif // HAL_PLATFORM_ETHERNET

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
test(ACM_05_prefer_moves_cloud_connection_when_set_to_wifi) {
    assertMoreOrEqual(s_availableNetworkCount, 2);
    bool ok = false;
    preferSwitchTest(WiFi, true /* connect */, ok);
    assertTrue(ok);
}
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY

#if HAL_PLATFORM_CELLULAR
test(ACM_06_prefer_moves_cloud_connection_when_set_to_cellular) {
    assertMoreOrEqual(s_availableNetworkCount, 2);
    bool ok = false;
    preferSwitchTest(Cellular, true /* connect */, ok);
    assertTrue(ok);
}
#endif // HAL_PLATFORM_CELLULAR

#if HAL_PLATFORM_ETHERNET
test(ACM_07_prefer_moves_cloud_connection_when_set_to_ethernet) {
    assertMoreOrEqual(s_availableNetworkCount, 2);
    if (!s_ethernetAvailable) {
        skip();
        return;
    }
    bool ok = false;
    preferSwitchTest(Ethernet, true /* connect */, ok);
    assertTrue(ok);
}
#endif // HAL_PLATFORM_ETHERNET

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
test(ACM_08_cloud_connection_moves_to_preferred_network_when_available_wifi) {
    assertMoreOrEqual(s_availableNetworkCount, 2);
    bool ok = false;
    preferSwitchTest(WiFi, false /* connect */, ok);
    assertTrue(ok);
}
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY

#if HAL_PLATFORM_CELLULAR
test(ACM_09_cloud_connection_moves_to_preferred_network_when_available_cellular) {
    assertMoreOrEqual(s_availableNetworkCount, 2);
    bool ok = false;
    preferSwitchTest(Cellular, false /* connect */, ok);
    assertTrue(ok);
}
#endif // HAL_PLATFORM_CELLULAR

#if HAL_PLATFORM_ETHERNET
test(ACM_10_cloud_connection_moves_to_preferred_network_when_available_ethernet) {
    assertMoreOrEqual(s_availableNetworkCount, 2);
    if (!s_ethernetAvailable) {
        skip();
        return;
    }
    bool ok = false;
    preferSwitchTest(Ethernet, false /* connect */, ok);
    assertTrue(ok);
}
#endif // HAL_PLATFORM_ETHERNET

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
test(ACM_11_cloud_connection_fails_over_automatically_from_wifi_and_back) {
    assertMoreOrEqual(s_availableNetworkCount, 2);
    bool ok = false;
    preferSwitchTest(WiFi, true /* connect */, ok);
    assertTrue(ok);
    WiFi.disconnect();
    assertTrue(waitFor(Particle.disconnected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertNotEqual(WiFi, Particle.connectionInterface());
    WiFi.connect();
    assertTrue(waitFor(WiFi.ready, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    for (int i = 0; i < 2; i++) {
        assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
        if (WiFi == Particle.connectionInterface()) {
            break;
        }
        delay(5000);
    }
    assertEqual(WiFi, Particle.connectionInterface());
}
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY

#if HAL_PLATFORM_CELLULAR
test(ACM_12_cloud_connection_fails_over_automatically_from_cellular_and_back) {
    assertMoreOrEqual(s_availableNetworkCount, 2);
    bool ok = false;
    preferSwitchTest(Cellular, true /* connect */, ok);
    assertTrue(ok);
    Cellular.disconnect();
    assertTrue(waitFor(Particle.disconnected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertNotEqual(Cellular, Particle.connectionInterface());
    Cellular.connect();
    assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    for (int i = 0; i < 2; i++) {
        assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
        if (Cellular == Particle.connectionInterface()) {
            break;
        }
        delay(5000);
    }
    assertEqual(Cellular, Particle.connectionInterface());
}
#endif // HAL_PLATFORM_CELLULAR

#if HAL_PLATFORM_ETHERNET
test(ACM_13_cloud_connection_fails_over_automatically_from_ethernet_and_back) {
    assertMoreOrEqual(s_availableNetworkCount, 2);
    if (!s_ethernetAvailable) {
        skip();
        return;
    }
    bool ok = false;
    preferSwitchTest(Ethernet, true /* connect */, ok);
    assertTrue(ok);
    Ethernet.disconnect();
    assertTrue(waitFor(Particle.disconnected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertNotEqual(Ethernet, Particle.connectionInterface());
    Ethernet.connect();
    assertTrue(waitFor(Ethernet.ready, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    for (int i = 0; i < 2; i++) {
        assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
        if (Ethernet == Particle.connectionInterface()) {
            break;
        }
        delay(5000);
    }
    assertEqual(Ethernet, Particle.connectionInterface());
}
#endif // HAL_PLATFORM_ETHERNET

#if HAL_PLATFORM_ETHERNET
test(ACM_14_disable_ethernet) {
    if (!s_ethernetAvailable) {
        s_skipNonEthernet = true;
        return;
    }
    System.disableFeature(FEATURE_ETHERNET_DETECTION);
    // Notify about a pending reset
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 5000));
    System.reset();
}

test(ACM_15_non_eth_prepare) {
    if (s_skipNonEthernet) {
        skip();
        return;
    }

    Particle.disconnect();

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
    WiFi.on();
    WiFi.disconnect();
    s_availableNetworkCount++;
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
#if HAL_PLATFORM_CELLULAR
    Cellular.on();
    Cellular.disconnect();
    s_availableNetworkCount++;
#endif // HAL_PLATFORM_CELLULAR

    assertEqual(Network, Particle.connectionInterface());
}

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
test(ACM_16_non_eth_particle_connect_uses_preferred_wifi_network) {
    if (s_skipNonEthernet) {
        skip();
        return;
    }
    assertMoreOrEqual(s_availableNetworkCount, 2);
    bool ok = false;
    preferConnectTest(WiFi, ok);
    assertTrue(ok);
}
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY

#if HAL_PLATFORM_CELLULAR
test(ACM_17_non_eth_particle_connect_uses_preferred_cellular_network) {
    if (s_skipNonEthernet) {
        skip();
        return;
    }
    assertMoreOrEqual(s_availableNetworkCount, 2);
    bool ok = false;
    preferConnectTest(Cellular, ok);
    assertTrue(ok);
}
#endif // HAL_PLATFORM_CELLULAR

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
test(ACM_18_non_eth_prefer_moves_cloud_connection_when_set_to_wifi) {
    if (s_skipNonEthernet) {
        skip();
        return;
    }
    assertMoreOrEqual(s_availableNetworkCount, 2);
    bool ok = false;
    preferSwitchTest(WiFi, true /* connect */, ok);
    assertTrue(ok);
}
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY

#if HAL_PLATFORM_CELLULAR
test(ACM_19_non_eth_prefer_moves_cloud_connection_when_set_to_cellular) {
    if (s_skipNonEthernet) {
        skip();
        return;
    }
    assertMoreOrEqual(s_availableNetworkCount, 2);
    bool ok = false;
    preferSwitchTest(Cellular, true /* connect */, ok);
    assertTrue(ok);
}
#endif // HAL_PLATFORM_CELLULAR

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
test(ACM_20_non_eth_cloud_connection_moves_to_preferred_network_when_available_wifi) {
    if (s_skipNonEthernet) {
        skip();
        return;
    }
    assertMoreOrEqual(s_availableNetworkCount, 2);
    bool ok = false;
    preferSwitchTest(WiFi, false /* connect */, ok);
    assertTrue(ok);
}
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY

#if HAL_PLATFORM_CELLULAR
test(ACM_21_non_eth_cloud_connection_moves_to_preferred_network_when_available_cellular) {
    if (s_skipNonEthernet) {
        skip();
        return;
    }
    assertMoreOrEqual(s_availableNetworkCount, 2);
    bool ok = false;
    preferSwitchTest(Cellular, false /* connect */, ok);
    assertTrue(ok);
}
#endif // HAL_PLATFORM_CELLULAR

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
test(ACM_22_non_eth_cloud_connection_fails_over_automatically_from_wifi_and_back) {
    if (s_skipNonEthernet) {
        skip();
        return;
    }
    assertMoreOrEqual(s_availableNetworkCount, 2);
    bool ok = false;
    preferSwitchTest(WiFi, true /* connect */, ok);
    assertTrue(ok);
    WiFi.disconnect();
    assertTrue(waitFor(Particle.disconnected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertNotEqual(WiFi, Particle.connectionInterface());
    WiFi.connect();
    assertTrue(waitFor(WiFi.ready, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    for (int i = 0; i < 2; i++) {
        assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
        if (WiFi == Particle.connectionInterface()) {
            break;
        }
        delay(5000);
    }
    assertEqual(WiFi, Particle.connectionInterface());
}
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY

#if HAL_PLATFORM_CELLULAR
test(ACM_23_non_eth_cloud_connection_fails_over_automatically_from_cellular_and_back) {
    if (s_skipNonEthernet) {
        skip();
        return;
    }
    assertMoreOrEqual(s_availableNetworkCount, 2);
    bool ok = false;
    preferSwitchTest(Cellular, true /* connect */, ok);
    assertTrue(ok);
    Cellular.disconnect();
    assertTrue(waitFor(Particle.disconnected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertNotEqual(Cellular, Particle.connectionInterface());
    Cellular.connect();
    assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    for (int i = 0; i < 2; i++) {
        assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
        if (Cellular == Particle.connectionInterface()) {
            break;
        }
        delay(5000);
    }
    assertEqual(Cellular, Particle.connectionInterface());
}
#endif // HAL_PLATFORM_CELLULAR

#endif // HAL_PLATFORM_ETHERNET

test(ACM_99_cleanup) {
#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
    WiFi.connect();
#endif // HAL_PLATFORM_WIFI
#if HAL_PLATFORM_CELLULAR
    Cellular.connect();
#endif // HAL_PLATFORM_CELLULAR
#if HAL_PLATFORM_ETHERNET
    Ethernet.connect();
#endif // HAL_PLATFORM_ETHERNET

    Particle.connect();
    assertEqual(Network, Network.prefer(false));
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
}
