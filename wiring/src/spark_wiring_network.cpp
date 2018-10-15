/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "spark_wiring_network.h"

#include "spark_wiring_wifi.h"
#include "spark_wiring_cellular.h"
#include "spark_wiring_mesh.h"

namespace spark {

NetworkClass& NetworkClass::from(network_interface_t nif) {
    switch (nif) {
#if Wiring_Mesh
    case NETWORK_INTERFACE_MESH:
        return Mesh;
#endif
#if Wiring_Ethernet
    case NETWORK_INTERFACE_ETHERNET:
        return Network; // FIXME
#endif
#if Wiring_WiFi
    case NETWORK_INTERFACE_WIFI_STA:
        return WiFi;
    case NETWORK_INTERFACE_WIFI_AP:
        return Network; // FIXME
#endif
#if Wiring_Cellular
    case NETWORK_INTERFACE_CELLULAR:
        return Cellular;
#endif
    default:
        return Network;
    }
}

} // spark
