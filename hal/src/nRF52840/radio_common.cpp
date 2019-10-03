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

#include "radio_common.h"

#include "gpio_hal.h"
#include "dct.h"

#include "check.h"

namespace particle {

namespace {

#if HAL_PLATFORM_RADIO_ANTENNA_INTERNAL
const auto DEFAULT_ANTENNA = RadioAntenna::INTERNAL;
#elif HAL_PLATFORM_RADIO_ANTENNA_EXTERNAL
const auto DEFAULT_ANTENNA = RadioAntenna::EXTERNAL;
#else
#error "Unsupported platform"
#endif

#if HAL_PLATFORM_RADIO_ANTENNA_INTERNAL

int setInternalAntenna() {
#if PLATFORM_ID == PLATFORM_ARGON
    HAL_Pin_Mode(ANTSW1, OUTPUT);
    HAL_Pin_Mode(ANTSW2, OUTPUT);
    HAL_GPIO_Write(ANTSW1, 0);
    HAL_GPIO_Write(ANTSW2, 1);
#elif PLATFORM_ID == PLATFORM_BORON
    HAL_Pin_Mode(ANTSW1, OUTPUT);
    HAL_GPIO_Write(ANTSW1, 1);
#elif PLATFORM_ID == PLATFORM_XENON
    HAL_Pin_Mode(ANTSW1, OUTPUT);
    HAL_Pin_Mode(ANTSW2, OUTPUT);
    HAL_GPIO_Write(ANTSW1, 1);
    HAL_GPIO_Write(ANTSW2, 0);
#else
#error "Unsupported platform"
#endif
    return 0;
}

#endif // HAL_PLATFORM_RADIO_ANTENNA_INTERNAL

#if HAL_PLATFORM_RADIO_ANTENNA_EXTERNAL

int setExtenalAntenna() {
#if PLATFORM_ID == PLATFORM_ARGON
    HAL_Pin_Mode(ANTSW1, OUTPUT);
    HAL_Pin_Mode(ANTSW2, OUTPUT);
    HAL_GPIO_Write(ANTSW1, 1);
    HAL_GPIO_Write(ANTSW2, 0);
#elif PLATFORM_ID == PLATFORM_BORON
    HAL_Pin_Mode(ANTSW1, OUTPUT);
    HAL_GPIO_Write(ANTSW1, 0);
#elif PLATFORM_ID == PLATFORM_XENON
    HAL_Pin_Mode(ANTSW1, OUTPUT);
    HAL_Pin_Mode(ANTSW2, OUTPUT);
    HAL_GPIO_Write(ANTSW1, 0);
    HAL_GPIO_Write(ANTSW2, 1);
// SoM platforms have only an external antenna
#elif PLATFORM_ID != PLATFORM_ASOM && PLATFORM_ID != PLATFORM_BSOM && PLATFORM_ID != PLATFORM_XSOM
#error "Unsupported platform"
#endif
    return 0;
}

#endif // HAL_PLATFORM_RADIO_ANTENNA_EXTERNAL

int setAntenna(RadioAntenna antenna) {
    if (antenna == RadioAntenna::DEFAULT) {
        antenna = DEFAULT_ANTENNA;
    }
    if (antenna == RadioAntenna::INTERNAL) {
#if HAL_PLATFORM_RADIO_ANTENNA_INTERNAL
        CHECK(setInternalAntenna());
#else
        return SYSTEM_ERROR_NOT_SUPPORTED;
#endif
    } else {
#if HAL_PLATFORM_RADIO_ANTENNA_EXTERNAL
        CHECK(setExtenalAntenna());
#else
        return SYSTEM_ERROR_NOT_SUPPORTED;
#endif
    }
    return 0;
}

} // namespace

int initRadioAntenna() {
    auto antenna = RadioAntenna::DEFAULT;
    uint8_t dctAntenna = 0xff;
    if (dct_read_app_data_copy(DCT_RADIO_ANTENNA_OFFSET, &dctAntenna, DCT_RADIO_ANTENNA_SIZE) < 0) {
        LOG(ERROR, "Unable to load antenna settings");
        // Use the default antenna
    } else if (dctAntenna == DCT_RADIO_ANTENNA_INTERNAL) {
        antenna = RadioAntenna::INTERNAL;
    } else if (dctAntenna == DCT_RADIO_ANTENNA_EXTERNAL) {
        antenna = RadioAntenna::EXTERNAL;
    }
    CHECK(setAntenna(antenna));
    return 0;
}

int setRadioAntenna(RadioAntenna antenna) {
    CHECK(setAntenna(antenna));
    uint8_t dctAntenna = 0xff;
    if (antenna == RadioAntenna::INTERNAL) {
        dctAntenna = DCT_RADIO_ANTENNA_INTERNAL;
    } else if (antenna == RadioAntenna::EXTERNAL) {
        dctAntenna = DCT_RADIO_ANTENNA_EXTERNAL;
    }
    if (dct_write_app_data(&dctAntenna, DCT_RADIO_ANTENNA_OFFSET, DCT_RADIO_ANTENNA_SIZE) < 0) {
        LOG(ERROR, "Unable to save antenna settings");
        return SYSTEM_ERROR_UNKNOWN;
    }
    return 0;
}

} // namespace particle
