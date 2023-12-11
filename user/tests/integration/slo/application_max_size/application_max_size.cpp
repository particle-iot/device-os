/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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
#include "test.h"

namespace {

#if HAL_PLATFORM_NRF52840
constexpr size_t FLASH_FILL_SIZE = 239 * 1024; // 239KB
#elif HAL_PLATFORM_RTL872X
constexpr size_t FLASH_FILL_SIZE = 1445 * 1024; // ~1.5MB
#else
#error "Unsupported platform"
#endif

const uint8_t flashFiller[FLASH_FILL_SIZE] __attribute__((used)) = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10
};

} // anonymous

test(APPLICATION_MAX_SIZE_00) {
    for (size_t i = 0; i < sizeof(flashFiller); i++) {
        LOG(NONE, "%u", flashFiller[i]);
    }
}
