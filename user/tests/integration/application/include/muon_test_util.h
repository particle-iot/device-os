/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#pragma once

#include "Particle.h"
#include "ifapi.h"
#include "ifapi_driver_specific.h"

namespace particle {
namespace test {

inline bool detectMuonBoard() {
#if HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
    constexpr uint8_t addrs[] = {
        0x28, // STUSB4500 USB PD chip
        0x69, // AM18x5 RTC
        0x48, // TMP112A temperature sensor
        0x6B, // BQ24195 PMIC
        0x36  // MAX17043 fuel gauge
    };
    Wire.begin();
    for (const auto addr : addrs) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() != 0) {
            return false;
        }
    }
    return true;
#else
    return false;
#endif
}

inline int configureMuonBoard() {
#if HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
    SystemPowerConfiguration powerConfig = System.getPowerConfiguration();
    powerConfig.auxiliaryPowerControlPin(D7).interruptPin(A7);
    int ret = System.setPowerConfiguration(powerConfig);
    if (ret != SYSTEM_ERROR_NONE) {
        return ret;
    }

    if_wiznet_pin_remap remap = {};
    remap.base.type = IF_WIZNET_DRIVER_SPECIFIC_PIN_REMAP;
    System.enableFeature(FEATURE_ETHERNET_DETECTION);
    remap.cs_pin = A3;
    remap.reset_pin = PIN_INVALID;
    remap.int_pin = A4;
    return if_request(nullptr, IF_REQ_DRIVER_SPECIFIC, &remap, sizeof(remap), nullptr);
#else
    return SYSTEM_ERROR_NONE;
#endif
}

inline int configureMuonExrtc() {
#if HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL && HAL_PLATFORM_AM18X5
    auto config = Am18x5Configuration()
            .i2c(Wire)
            .defaultTimeSource(true)
            .interruptPin(PIN_INVALID)
            .watchdogPin(PIN_INVALID)
            .clockSource(RtcClockSource::EXTERNAL)
            .capabilities(RtcCap::AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL);
    return Am18x5.enable(config);
#else
    return SYSTEM_ERROR_NONE;
#endif
}

} // namespace test
} // namespace particle
