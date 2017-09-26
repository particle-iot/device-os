/**
  Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

#pragma once

#include "spark_wiring_diagnostics.h"

namespace particle { namespace power {

class BatteryChargeDiagnosticData: public AbstractIntegerDiagnosticData {
public:
    BatteryChargeDiagnosticData(uint16_t id, const char* name = nullptr);
private:
    virtual int get(IntType& val) override; // AbstractIntegerDiagnosticData
};

typedef enum {
    BATTERY_STATE_UNKNOWN = 0,
    BATTERY_STATE_NOT_CHARGING = 1,
    BATTERY_STATE_CHARGING = 2,
    BATTERY_STATE_CHARGED = 3,
    BATTERY_STATE_DISCHARGING = 4,
    BATTERY_STATE_FAULT = 5
} BatteryState;

} } // particle::power

extern particle::power::BatteryChargeDiagnosticData g_batteryCharge;
extern particle::SimpleEnumDiagnosticData<particle::power::BatteryState> g_batteryState;

void system_power_management_init();
void system_power_management_update();
void system_power_management_sleep(bool sleep = true);
