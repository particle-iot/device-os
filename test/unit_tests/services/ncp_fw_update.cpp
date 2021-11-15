/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include "ncp_fw_update.h"
// #include "system_error.h"

// #include "mock/filesystem.h"
// #include "mock/ncp_fw_update.h"
// #include "util/random.h"

#include <catch2/catch.hpp>
#include <hippomocks.h>

#include <string>
// #include <unordered_map>


using namespace particle;

TEST_CASE("SaraNcpFwUpdate") {
    // MockRepository mocks;
    SECTION("init()") {
        SECTION("initialize callbacks") {
            SaraNcpFwUpdateCallbacks saraNcpFwUpdateCallbacks;
            memset(&saraNcpFwUpdateCallbacks, 0, sizeof(saraNcpFwUpdateCallbacks));
            // saraNcpFwUpdateCallbacks.size = sizeof(saraNcpFwUpdateCallbacks);
            // saraNcpFwUpdateCallbacks.system_get_flag = system_get_flag;
            // saraNcpFwUpdateCallbacks.spark_cloud_flag_connected = spark_cloud_flag_connected;
            // saraNcpFwUpdateCallbacks.spark_cloud_flag_connect = spark_cloud_flag_connect;
            // saraNcpFwUpdateCallbacks.spark_cloud_flag_disconnect = spark_cloud_flag_disconnect;
            // saraNcpFwUpdateCallbacks.publishEvent = publishEvent;
            // saraNcpFwUpdateCallbacks.system_mode = system_mode;
            // services::SaraNcpFwUpdate::instance()->init(saraNcpFwUpdateCallbacks);
            // CHECK(strcmp(n.init(), "path/to/file") == 0);
        }
    }
}
