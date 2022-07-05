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

#define PARTICLE_USE_UNSTABLE_API
#include "application.h"
#include "unit-test/unit-test.h"

#if HAL_PLATFORM_GEN != 3
#error "Unsupported platform"
#endif // HAL_PLATFORM_GEN != 3

// FIXME: there should be a way to filter out P2 vs other Gen 3 platforms in spec.js
#if HAL_PLATFORM_NRF52840

struct padded_module_info {
    uint8_t padding[0x4000];
    module_info_t info;
};

// This structure is placed at exactly where 128KB compat
// user module would be located due to 64KB alignment and padding
__attribute__((aligned(64 * 1024), used)) const padded_module_info brokenModule = {
    .padding = {},
    .info = {
        .module_start_address = (void*)0x123,
        .module_end_address = (void*)0x1234,
        .reserved = 0,
        .flags = 0,
        .module_version = 12345,
        .platform_id = 100,
        .module_function = 123,
        .module_index = 123,
        .dependency = {
            .module_function = MODULE_FUNCTION_RESOURCE,
            .module_index = 11,
            .module_version = 11
        },
        .dependency2 = {
            .module_function = MODULE_FUNCTION_RESOURCE,
            .module_index = 12,
            .module_version = 12
        }
    }
};

test(00_system_describe_does_not_contain_invalid_compat_user_app) {
    hal_system_info_t info = {};
    info.size = sizeof(info);
    system_info_get_unstable(&info, 0, nullptr);
    SCOPE_GUARD({
        system_info_free_unstable(&info, nullptr);
    });

    assertFalse(info.modules == nullptr);
    for (unsigned i = 0; i < info.module_count; i++) {
        assertFalse(!memcmp(&info.modules[i].info, &brokenModule.info, sizeof(module_info_t)));
    }
}
#endif // HAL_PLATFORM_NRF52840