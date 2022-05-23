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

#pragma once

#include "spark_wiring_platform.h"

#if HAL_PLATFORM_POWER_MANAGEMENT
#include "system_power.h"
#include "enumflags.h"
#include "spark_wiring_diagnostics.h"

namespace particle {

enum class SystemPowerFeature {
    NONE = 0,
    PMIC_DETECTION = HAL_POWER_PMIC_DETECTION,
    USE_VIN_SETTINGS_WITH_USB_HOST = HAL_POWER_USE_VIN_SETTINGS_WITH_USB_HOST,
    DISABLE = HAL_POWER_MANAGEMENT_DISABLE,
    DISABLE_CHARGING = HAL_POWER_CHARGE_STATE_DISABLE
};

class SystemPowerConfiguration {
public:

    SystemPowerConfiguration()
            : conf_{} {
        conf_.size = sizeof(conf_);
    }

    SystemPowerConfiguration(SystemPowerConfiguration&&) = default;
    SystemPowerConfiguration(const hal_power_config& conf) : conf_(conf) {}
    SystemPowerConfiguration& operator=(SystemPowerConfiguration&&) = default;

    SystemPowerConfiguration& powerSourceMinVoltage(uint16_t voltage) {
        conf_.vin_min_voltage = voltage;
        return *this;
    }

    uint16_t powerSourceMinVoltage() const {
        return conf_.vin_min_voltage;
    }

    SystemPowerConfiguration& powerSourceMaxCurrent(uint16_t current) {
        conf_.vin_max_current = current;
        return *this;
    }

    uint16_t powerSourceMaxCurrent() const {
        return conf_.vin_max_current;
    }

    SystemPowerConfiguration& batteryChargeVoltage(uint16_t voltage) {
        conf_.termination_voltage = voltage;
        return *this;
    }

    uint16_t batteryChargeVoltage() const {
        return conf_.termination_voltage;
    }

    SystemPowerConfiguration& batteryChargeCurrent(uint16_t current) {
        conf_.charge_current = current;
        return *this;
    }

    uint16_t batteryChargeCurrent() const {
        return conf_.charge_current;
    }

    SystemPowerConfiguration& feature(EnumFlags<SystemPowerFeature> f) {
        conf_.flags |= f.value();
        return *this;
    }

    SystemPowerConfiguration& clearFeature(EnumFlags<SystemPowerFeature> f) {
        conf_.flags &= ~(f.value());
        return *this;
    }

    bool isFeatureSet(EnumFlags<SystemPowerFeature> f) const {
        return (conf_.flags & f.value()) ? true : false;
    }

    SystemPowerConfiguration& socBitPrecision(uint8_t bits) {
        conf_.soc_bits = bits;
        return *this;
    }

    uint8_t socBitPrecision() const {
        return conf_.soc_bits;
    }

    const hal_power_config* config() const {
        return &conf_;
    }
private:
    hal_power_config conf_;
};

} // particle

#endif // HAL_PLATFORM_POWER_MANAGEMENT