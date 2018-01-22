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

#include "system_tick_hal.h"
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
    BATTERY_STATE_FAULT = 5,
    BATTERY_STATE_DISCONNECTED = 6
} battery_state_t;

typedef enum {
    POWER_SOURCE_UNKNOWN = 0,
    POWER_SOURCE_VIN = 1,
    POWER_SOURCE_USB_HOST = 2,
    POWER_SOURCE_USB_ADAPTER = 3,
    POWER_SOURCE_USB_OTG = 4,
    POWER_SOURCE_BATTERY = 5
} power_source_t;

} } // particle::power

extern particle::power::BatteryChargeDiagnosticData g_batteryCharge;
extern particle::SimpleEnumDiagnosticData<particle::power::battery_state_t> g_batteryState;
extern particle::SimpleEnumDiagnosticData<particle::power::power_source_t> g_powerSource;

void system_power_management_init();
void system_power_management_sleep(bool sleep = true);
