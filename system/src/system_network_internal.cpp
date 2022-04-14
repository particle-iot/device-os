/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
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

#include "system_network_internal.h"

#include "system_cloud.h"

namespace particle {

namespace {

[[gnu::unused]] // Suppress a warning on the newhal platform
bool turnOffNetworkIfNeeded(network_interface_index iface) {
    if (network_ready(iface, NETWORK_READY_TYPE_ANY, nullptr) || network_connecting(iface, 0, nullptr)) {
        network_off(iface, 0, 0, nullptr);
        return true;
    }
    return false;
}

} // namespace

void resetNetworkInterfaces() {
    // FIXME: network_off() on Gen 3 disconnects the cloud only if the interface is set to NETWORK_INTERFACE_ALL
    cloud_disconnect(CLOUD_DISCONNECT_GRACEFULLY, NETWORK_DISCONNECT_REASON_RESET);
    // TODO: There's no cross-platform API to enumerate available network interfaces
#if HAL_PLATFORM_ETHERNET
    const bool resetEthernet = turnOffNetworkIfNeeded(NETWORK_INTERFACE_ETHERNET);
#endif // HAL_PLATFORM_ETHERNET
#if HAL_PLATFORM_CELLULAR
    const bool resetCellular = turnOffNetworkIfNeeded(NETWORK_INTERFACE_CELLULAR);
#endif // HAL_PLATFORM_CELLULAR
#if HAL_PLATFORM_WIFI
    const bool resetWifi = turnOffNetworkIfNeeded(NETWORK_INTERFACE_WIFI_STA);
#endif // HAL_PLATFORM_WIFI
#if HAL_PLATFORM_ETHERNET
    if (resetEthernet) {
        network_connect(NETWORK_INTERFACE_ETHERNET, 0, 0, nullptr);
    }
#endif // HAL_PLATFORM_ETHERNET
#if HAL_PLATFORM_CELLULAR
    if (resetCellular) {
        network_connect(NETWORK_INTERFACE_CELLULAR, 0, 0, nullptr);
    }
#endif // HAL_PLATFORM_CELLULAR
#if HAL_PLATFORM_WIFI
    if (resetWifi) {
        network_connect(NETWORK_INTERFACE_WIFI_STA, 0, 0, nullptr);
    }
#endif // HAL_PLATFORM_WIFI
}

} // namespace particle
