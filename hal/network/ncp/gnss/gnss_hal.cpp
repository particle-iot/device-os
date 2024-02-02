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
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "check.h"
#include "delay_hal.h"
#include "service_debug.h"
#include "logging.h"
#include "gnss_hal.h"

#include "network/ncp/cellular/cellular_network_manager.h"
#include "network/ncp/cellular/cellular_ncp_client.h"
#include "network/ncp/cellular/ncp.h"
#include "network/ncp_client/quectel/quectel_ncp_client.h"

using namespace particle;

// Just for testing purposes
int hal_gnss_init(void* reserved) {
    const auto mgr = cellularNetworkManager();
    CHECK_TRUE(mgr, SYSTEM_ERROR_UNKNOWN);
    const auto client = reinterpret_cast<QuectelNcpClient*>(mgr->ncpClient());
    CHECK_TRUE(client, SYSTEM_ERROR_UNKNOWN);

    GnssNcpClientConfig config = {};
    config.enableGpsOneXtra(true);
    client->gnssConfig(config);

    return 0;
}
