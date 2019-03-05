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

#include "rng_hal.h"

#include "logging.h"

#include "nrf_drv_rng.h"

void HAL_RNG_Configuration() {
    const auto ret = nrf_drv_rng_init(nullptr);
    if (ret != NRF_SUCCESS) {
        LOG(ERROR, "nrf_drv_rng_init() failed: %d", (int)ret);
    }
}

uint32_t HAL_RNG_GetRandomNumber() {
    uint32_t val = 0;
    nrf_drv_rng_block_rand((uint8_t*)&val, sizeof(val));
    return val;
}
