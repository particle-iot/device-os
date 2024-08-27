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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef POWER_MODULE_H
#define POWER_MODULE_H

#include "Particle.h"

namespace particle {

class PowerModule {
public:
    int begin();
    int end();
    int getFuelGaugeVersion();
    int setAlertThreshold(uint8_t threshold);
    float getSoc();
    bool isBatLevelLow();
    int clearFuelGaugeAlert();

    int getPmicVersion();

    int auxPowerControl(bool enable);

    static PowerModule& getInstance();
    static uint8_t auxEnPin() { return auxEnPin_; }
    static uint8_t intPin() { return intPin_; }

    bool initialized_;
    static constexpr uint8_t intPin_ = A7;
    static constexpr uint8_t auxEnPin_ = D7;

private:
    PowerModule();
    ~PowerModule();
}; // class PowerModule

} // namespace particle

#endif // POWER_MODULE_H