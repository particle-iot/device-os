#include "system_power.h"
#include "spark_wiring_platform.h"
#include "spark_wiring_fuel.h"
#include "spark_wiring_diagnostics.h"
#include "spark_wiring_fixed_point.h"
#include "debug.h"

#if HAL_PLATFORM_POWER_MANAGEMENT

#include "system_power_manager.h"

namespace particle { namespace power {

BatteryChargeDiagnosticData::BatteryChargeDiagnosticData(uint16_t id, const char* name) :
    AbstractIntegerDiagnosticData(id, name) {
}

int BatteryChargeDiagnosticData::get(IntType& val) {
    // Do not report battery SoC in disconnected and unknown states
    if (g_batteryState == BATTERY_STATE_DISCONNECTED || g_batteryState == BATTERY_STATE_UNKNOWN) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    FuelGauge fuel(true);
    // Call begin here just in case to initialize I2C if needed
    fuel.begin();
    float soc = fuel.getNormalizedSoC();
    val = particle::FixedPointUQ<8, 8>(soc);
    return SYSTEM_ERROR_NONE;
}

} } // particle::power

using namespace particle;
using namespace particle::power;

BatteryChargeDiagnosticData g_batteryCharge(DIAG_ID_SYSTEM_BATTERY_CHARGE, DIAG_NAME_SYSTEM_BATTERY_CHARGE);
SimpleEnumDiagnosticData<battery_state_t> g_batteryState(DIAG_ID_SYSTEM_BATTERY_STATE, DIAG_NAME_SYSTEM_BATTERY_STATE, BATTERY_STATE_UNKNOWN);
SimpleEnumDiagnosticData<power_source_t> g_powerSource(DIAG_ID_SYSTEM_POWER_SOURCE, DIAG_NAME_SYSTEM_POWER_SOURCE, POWER_SOURCE_UNKNOWN);

void system_power_management_init()
{
    PowerManager::instance()->init();
}

void system_power_management_sleep(bool sleep) {
    PowerManager::instance()->sleep(sleep);
}

#else /* !HAL_PLATFORM_POWER_MANAGEMENT */

void system_power_management_init() {
}

void system_power_management_sleep(bool sleep) {
}

#endif /* HAL_PLATFORM_POWER_MANAGEMENT */
