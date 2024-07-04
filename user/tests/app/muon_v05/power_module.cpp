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


#include "power_module.h"

using namespace particle;

PowerModule::PowerModule()
    : initialized_(false) {

}

PowerModule::~PowerModule() {

}

PowerModule& PowerModule::getInstance() {
    static PowerModule pm;
    return pm;
}

int PowerModule::begin() {
    CHECK_FALSE(initialized_, SYSTEM_ERROR_NONE);
    FuelGauge fuel;
    fuel.wakeup();
    fuel.clearAlert();
    pinMode(intPin_, INPUT_PULLUP);
    pinMode(auxEnPin_, OUTPUT);
    initialized_ = true;
    return SYSTEM_ERROR_NONE;
}

int PowerModule::end() {
    CHECK_TRUE(initialized_, SYSTEM_ERROR_NONE);
    initialized_ = false;
    return SYSTEM_ERROR_NONE;
}

int PowerModule::getFuelGaugeVersion() {
    FuelGauge fuel;
    fuel.wakeup();
    delay(1000);
    return fuel.getVersion();
}

int PowerModule::setAlertThreshold(uint8_t threshold) {
    FuelGauge fuel;
    fuel.wakeup();
    fuel.setAlertThreshold(threshold);
    return SYSTEM_ERROR_NONE;
}

float PowerModule::getSoc() {
    FuelGauge fuelGauge;
    return fuelGauge.getSoC();
}

bool PowerModule::isBatLevelLow() {
    return digitalRead(intPin_) == LOW;
}

int PowerModule::clearFuelGaugeAlert() {
    FuelGauge fuel;
    fuel.wakeup();
    fuel.clearAlert();
    return SYSTEM_ERROR_NONE;
}

int PowerModule::getPmicVersion() {
    PMIC power(true);
    power.begin();
    return power.getVersion();
}

int PowerModule::auxPowerControl(bool enable) {
    CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
    digitalWrite(auxEnPin_, enable);
    return SYSTEM_ERROR_NONE;
}
