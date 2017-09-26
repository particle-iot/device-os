#include "system_power.h"
#include "spark_wiring_platform.h"
#include "spark_wiring_power.h"
#include "spark_wiring_fuel.h"
#include "spark_wiring_diagnostics.h"
#include "spark_wiring_fixed_point.h"
#include "debug.h"

#if Wiring_Cellular == 1
/* flag used to initiate system_power_management_update() from main thread */
static volatile bool SYSTEM_POWER_MGMT_UPDATE = true;

namespace particle { namespace power {

BatteryChargeDiagnosticData::BatteryChargeDiagnosticData(uint16_t id, const char* name) :
    AbstractIntegerDiagnosticData(id, name) {
}

int BatteryChargeDiagnosticData::get(IntType& val) {
    FuelGauge fuel(true);
    float soc = fuel.getSoC();
    val = particle::FixedPointUQ<8, 8>(soc);
    return SYSTEM_ERROR_NONE;
}

} } // particle::power

using namespace particle;
using namespace particle::power;

BatteryChargeDiagnosticData g_batteryCharge(DIAG_ID_SYSTEM_BATTERY_CHARGE, DIAG_ID_SYSTEM_BATTERY_CHARGE_NAME);

/*******************************************************************************
 * Function Name  : Power_Management_Handler
 * Description    : Sets default power management IC charging rate when USB
                    input power source changes or low battery indicated by
                    fuel gauge IC.
 * Output         : SYSTEM_POWER_MGMT_UPDATE is set to true.
 *******************************************************************************/
extern "C" void Power_Management_Handler(void)
{
    SYSTEM_POWER_MGMT_UPDATE = true;
}

void system_power_management_init()
{
    INFO("Power Management Initializing.");
    PMIC power(true);
    FuelGauge fuel(true);
    power.begin();
    power.disableWatchdog();
    power.disableDPDM();
    // power.setInputVoltageLimit(4360); // default
    power.setInputCurrentLimit(900);     // 900mA
    power.setChargeCurrent(0,0,0,0,0,0); // 512mA
    power.setChargeVoltage(4112);        // 4.112V termination voltage
    fuel.wakeup();
    fuel.setAlertThreshold(10); // Low Battery alert at 10% (about 3.6V)
    fuel.clearAlert(); // Ensure this is cleared, or interrupts will never occur
    INFO("State of Charge: %-6.2f%%", fuel.getSoC());
    INFO("Battery Voltage: %-4.2fV", fuel.getVCell());
    attachInterrupt(LOW_BAT_UC, Power_Management_Handler, FALLING);
}

void system_power_management_update()
{
    if (SYSTEM_POWER_MGMT_UPDATE) {
        SYSTEM_POWER_MGMT_UPDATE = false;
        PMIC power(true);
        FuelGauge fuel(true);
        power.begin();
        power.setInputCurrentLimit(900);     // 900mA
        power.setChargeCurrent(0,0,0,0,0,0); // 512mA
        static bool lowBattEventNotified = false; // Whether 'low_battery' event was generated already
        static bool wasCharging = false; // Whether the battery was charging last time when this function was called
        const uint8_t status = power.getSystemStatus();
        const bool charging = (status >> 4) & 0x03;
        if (charging && !wasCharging) { // Check if the battery has started to charge
            lowBattEventNotified = false; // Allow 'low_battery' event to be generated again
        }
        wasCharging = charging;
        bool LOWBATT = fuel.getAlert();
        if (LOWBATT) {
            fuel.clearAlert(); // Clear the Low Battery Alert flag if set
            if (!lowBattEventNotified) {
                lowBattEventNotified = true;
                system_notify_event(low_battery);
            }
        }
//        if (LOG_ENABLED(INFO)) {
//              INFO(" %s", (LOWBATT)?"Low Battery Alert":"PMIC Interrupt");
//        }
#if defined(DEBUG_BUILD) && 0
        if (LOG_ENABLED(TRACE)) {
            uint8_t stat = power.getSystemStatus();
            uint8_t fault = power.getFault();
            uint8_t vbus_stat = stat >> 6; // 0 – Unknown (no input, or DPDM detection incomplete), 1 – USB host, 2 – Adapter port, 3 – OTG
            uint8_t chrg_stat = (stat >> 4) & 0x03; // 0 – Not Charging, 1 – Pre-charge (<VBATLOWV), 2 – Fast Charging, 3 – Charge Termination Done
            bool dpm_stat = stat & 0x08;   // 0 – Not DPM, 1 – VINDPM or IINDPM
            bool pg_stat = stat & 0x04;    // 0 – Not Power Good, 1 – Power Good
            bool therm_stat = stat & 0x02; // 0 – Normal, 1 – In Thermal Regulation
            bool vsys_stat = stat & 0x01;  // 0 – Not in VSYSMIN regulation (BAT > VSYSMIN), 1 – In VSYSMIN regulation (BAT < VSYSMIN)
            bool wd_fault = fault & 0x80;  // 0 – Normal, 1- Watchdog timer expiration
            uint8_t chrg_fault = (fault >> 4) & 0x03; // 0 – Normal, 1 – Input fault (VBUS OVP or VBAT < VBUS < 3.8 V),
                                                      // 2 - Thermal shutdown, 3 – Charge Safety Timer Expiration
            bool bat_fault = fault & 0x08;    // 0 – Normal, 1 – BATOVP
            uint8_t ntc_fault = fault & 0x07; // 0 – Normal, 5 – Cold, 6 – Hot
            DEBUG_D("[ PMIC STAT ] VBUS:%d CHRG:%d DPM:%d PG:%d THERM:%d VSYS:%d\r\n", vbus_stat, chrg_stat, dpm_stat, pg_stat, therm_stat, vsys_stat);
            DEBUG_D("[ PMIC FAULT ] WATCHDOG:%d CHRG:%d BAT:%d NTC:%d\r\n", wd_fault, chrg_fault, bat_fault, ntc_fault);
            delay(50);
        }
#endif
    }
}
#endif
