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
#include "power_hal.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

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

void system_power_management_init();
void system_power_management_sleep(bool fuelGaugeSleep);
void system_power_management_wakeup();
int system_power_management_set_config(const hal_power_config* conf, void* reserved);
int system_power_management_get_config(hal_power_config* conf, void* reserved);

#ifdef __cplusplus
}

namespace particle { namespace power {

class BatteryChargeDiagnosticData: public AbstractIntegerDiagnosticData {
public:
    BatteryChargeDiagnosticData(uint16_t id, const char* name = nullptr);
private:
    virtual int get(IntType& val) override; // AbstractIntegerDiagnosticData
};

using battery_state_t = ::battery_state_t;
using power_source_t = ::power_source_t;

// Exposed for tests
constexpr uint16_t DEFAULT_INPUT_CURRENT_LIMIT = 900; // 900mA
constexpr uint16_t DEFAULT_INPUT_VOLTAGE_LIMIT = 3880; // 3.88V
constexpr uint16_t DEFAULT_CHARGE_CURRENT = 896; // 896mA
constexpr uint16_t DEFAULT_TERMINATION_VOLTAGE = 4112; // 4.112V
constexpr uint8_t DEFAULT_SOC_18_BIT_PRECISION = 18; // 18 is default, but may be 18 or 19 when a custom model is loaded
constexpr uint8_t SOC_19_BIT_PRECISION = 19; // 19 is the only other valid value, for now

} } // particle::power

extern particle::power::BatteryChargeDiagnosticData g_batteryCharge;
extern particle::SimpleEnumDiagnosticData<particle::power::battery_state_t> g_batteryState;
extern particle::SimpleEnumDiagnosticData<particle::power::power_source_t> g_powerSource;

#endif // __cplusplus