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

test(ACM_01_PREPARE) {
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

    assertEqual(Network, Particle.connectionInterface());
}

test(ACM_02_particle_connect_uses_preferred_network) {
    auto expectedNetwork = Network;

#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY
    expectedNetwork = WiFi;
    WiFi.prefer();
    WiFi.connect();
    assertTrue(waitFor(WiFi.ready, WIFI_CONNECT_TIMEOUT_MS));
#endif // HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_SCAN_ONLY

#if HAL_PLATFORM_CELLULAR
    expectedNetwork = Cellular;
    Cellular.prefer();
    Cellular.connect();
    assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_CELLULAR_CONN_TIMEOUT));
#endif // HAL_PLATFORM_CELLULAR

    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertEqual(Particle.connectionInterface(), expectedNetwork);
}


#if PLATFORM_ID == PLATFORM_MSOM
test(ACM_03_prefer_one_network_only) {
    WiFi.prefer();
    assertTrue(WiFi.isPreferred());
    assertFalse(Cellular.isPreferred());
    assertFalse(Ethernet.isPreferred());
    Cellular.prefer();
    assertTrue(Cellular.isPreferred());
    assertFalse(WiFi.isPreferred());
    assertFalse(Ethernet.isPreferred());
}

test(ACM_04_prefer_moves_cloud_connection_when_set) {
    // Default preference
    Network.prefer();
    
    // Connect all interfaces
    Network.connect();
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    auto connectedNetwork = Particle.connectionInterface();

    // Prefer whatever network we did not connect on in order to force the cloud connection to switch
    if (connectedNetwork == Cellular) {
        assertTrue(waitFor(WiFi.ready, WIFI_CONNECT_TIMEOUT_MS));
        WiFi.prefer();
    } else {
        assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_CELLULAR_CONN_TIMEOUT));
        Cellular.prefer();
    }

    // Verify that we disconnect and reconnect on the newly preferred network
    assertTrue(waitForNot(Particle.connected, DISCONNECT_TIMEOUT_MS));
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertNotEqual(connectedNetwork, Particle.connectionInterface());
}

test(ACM_05_cloud_connection_moves_to_preferred_network_when_available) {
    // Disconnect WiFi
    WiFi.disconnect();
    assertTrue(waitForNot(WiFi.ready, DISCONNECT_TIMEOUT_MS));
    Particle.disconnect();
    assertTrue(waitForNot(Particle.connected, DISCONNECT_TIMEOUT_MS));
    
    // Connect Cellular
    Cellular.connect();
    assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_CELLULAR_CONN_TIMEOUT));
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    // Verify we are using celluar
    assertEqual(Cellular, Particle.connectionInterface());

    // Prefer wifi, switch to it
    WiFi.prefer();
    WiFi.connect();
    assertTrue(waitFor(WiFi.ready, WIFI_CONNECT_TIMEOUT_MS));

    // We should see the particle socket disconnect and reconnect
    assertTrue(waitFor(Particle.disconnected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    // Verify we are on wifi now
    assertEqual(WiFi, Particle.connectionInterface());
} 

test(ACM_06_cloud_connection_fails_over_automatically) {
    Particle.connect();
    Cellular.connect();
    WiFi.connect();

    assertTrue(waitFor(Cellular.ready, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertTrue(waitFor(WiFi.ready, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    auto previousNetwork = Particle.connectionInterface();
    previousNetwork.disconnect();

    assertTrue(waitFor(Particle.disconnected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));

    assertNotEqual(previousNetwork, Particle.connectionInterface());
}
#endif // PLATFORM_ID == PLATFORM_MSOM

test(ACM_99_cleanup) {
#if HAL_PLATFORM_WIFI
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
}