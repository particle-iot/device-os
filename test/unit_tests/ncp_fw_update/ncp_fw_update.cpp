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
#include "system_mode.h"

// class SaraNcpFwUpdateTest : public SaraNcpFwUpdate {
// public:
// //
// };

using namespace particle;

TEST_CASE("SaraNcpFwUpdate") {
    SECTION("init()") {
        SECTION("initialize callbacks") {
            SaraNcpFwUpdateCallbacks saraNcpFwUpdateCallbacks = {};
            saraNcpFwUpdateCallbacks.size = sizeof(saraNcpFwUpdateCallbacks);
            saraNcpFwUpdateCallbacks.system_get_flag = nullptr;
            saraNcpFwUpdateCallbacks.spark_cloud_flag_connected = nullptr;
            saraNcpFwUpdateCallbacks.spark_cloud_flag_connect = nullptr;
            saraNcpFwUpdateCallbacks.spark_cloud_flag_disconnect = nullptr;
            saraNcpFwUpdateCallbacks.publishEvent = nullptr;
            saraNcpFwUpdateCallbacks.system_mode = nullptr;

            MockRepository mocks;

            mocks.ExpectCallFunc(system_mode).Do([&]() -> System_Mode_TypeDef {
                return SAFE_MODE;
            });
            services::SaraNcpFwUpdate::instance()->init(saraNcpFwUpdateCallbacks);
        }
    }
}
