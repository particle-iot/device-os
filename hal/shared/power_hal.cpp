/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include "power_hal.h"
#include "dct.h"
#include "check.h"
#include "system_error.h"
#include <algorithm>

int hal_power_load_config(hal_power_config* conf, void* reserved) {
    CHECK_TRUE(conf, SYSTEM_ERROR_INVALID_ARGUMENT);
    int err = dct_read_app_data_copy(DCT_POWER_CONFIG_OFFSET, conf, conf->size);
    if (!err) {
        // Invert first byte of the flags to keep compatibility for HAL_POWER_PMIC_DETECTION flag
        uint32_t inverted = (~conf->flags) & 0x000000ff;
        conf->flags &= 0xffffff00;
        conf->flags |= inverted;
    }

    return err;
}

int hal_power_store_config(const hal_power_config* conf, void* reserved) {
    CHECK_TRUE(conf, SYSTEM_ERROR_INVALID_ARGUMENT);

    hal_power_config current = {};
    current.size = sizeof(current);
    CHECK(hal_power_load_config(&current, nullptr));

    if (memcmp(&current, conf, std::min((size_t)std::min(current.size, conf->size), sizeof(hal_power_config)))) {
        // Invert first byte of the flags to keep compatibility for HAL_POWER_PMIC_DETECTION flag
        uint32_t inverted = ~(conf->flags) & 0x000000ff;
        hal_power_config tmp = *conf;
        tmp.flags &= 0xffffff00;
        tmp.flags |= inverted;
        return dct_write_app_data(&tmp, DCT_POWER_CONFIG_OFFSET, sizeof(hal_power_config));
    }
    return SYSTEM_ERROR_NONE;
}
