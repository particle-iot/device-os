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

BatteryChargeDiagnosticData g_batteryCharge(DIAG_ID_SYSTEM_BATTERY_CHARGE, "sys.battCharge");
SimpleEnumDiagnosticData<BatteryState> g_batteryState(DIAG_ID_SYSTEM_BATTERY_STATE, "sys.battState", BATTERY_STATE_UNKNOWN);

static bool s_lowBattEventEnabled = true;
static system_tick_t s_possibleFaultTimestamp = 0;
static uint32_t s_possibleFaultCounter = 0;

void system_power_log_stat(uint8_t stat, uint8_t fault);
void system_power_handle_state_change(BatteryState from, BatteryState to, bool low);

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

void system_power_init_default_params() {
    PMIC power(true);
    power.begin();
    power.disableWatchdog();
    power.disableDPDM();
    // power.setInputVoltageLimit(4360); // default
    power.setInputCurrentLimit(900);     // 900mA
    power.setChargeCurrent(0,0,0,0,0,0); // 512mA
    power.setChargeVoltage(4112);        // 4.112V termination voltage
    power.enableCharging();
}

void system_power_management_init()
{
    INFO("Power Management Initializing.");
    system_power_init_default_params();
    FuelGauge fuel(true);
    fuel.wakeup();
    fuel.setAlertThreshold(10); // Low Battery alert at 10% (about 3.6V)
    fuel.clearAlert(); // Ensure this is cleared, or interrupts will never occur
    INFO("State of Charge: %-6.2f%%", fuel.getSoC());
    INFO("Battery Voltage: %-4.2fV", fuel.getVCell());
    attachInterrupt(LOW_BAT_UC, Power_Management_Handler, FALLING);

    system_power_management_update();
}

void system_power_management_sleep(bool sleep) {
    // When going into sleep we do not want to exceed the default charging parameters set
    // by system_power_init_default_params(), which will be reset in case we are in a fault mode with
    // PMIC watchdog enabled. Reset to the defaults and disable watchdog before going into sleep.
    if (sleep && g_batteryState == BATTERY_STATE_FAULT) {
        system_power_init_default_params();
        SYSTEM_POWER_MGMT_UPDATE = true;
    }
}

void system_power_management_update()
{
    if (!SYSTEM_POWER_MGMT_UPDATE) {
        return;
    }

    SYSTEM_POWER_MGMT_UPDATE = false;

    PMIC power(true);
    FuelGauge fuel(true);
    power.begin();

    // In order to read the current fault status, the host has to read REG09 two times
    // consecutively. The 1st reads fault register status from the last read and the 2nd
    // reads the current fault register status.
    const uint8_t lastFault = power.getFault();
    const uint8_t curFault = power.getFault();

    const uint8_t status = power.getSystemStatus();

    // Watchdog fault
    if ((curFault | lastFault) & 0x80) {
        // Restore parameters
        system_power_init_default_params();
    }

    BatteryState state = BATTERY_STATE_UNKNOWN;

    // Deduce current battery state
    const uint8_t chrg_stat = (status >> 4) & 0b11;
    if (chrg_stat) {
        // Charging or charged
        if (chrg_stat == 0b11) {
            state = BATTERY_STATE_CHARGED;
        } else {
            state = BATTERY_STATE_CHARGING;
        }
    } else {
        // For now we only know that the battery is not charging
        state = BATTERY_STATE_NOT_CHARGING;
        // Now we need to deduce whether it is NOT_CHARGING, DISCHARGING, or in a FAULT state
        // const uint8_t chrg_fault = (curFault >> 4) & 0b11;
        const uint8_t bat_fault = (curFault >> 3) & 0b01;
        // const uint8_t ntc_fault = curFault & 0b111;
        const uint8_t pwr_good = (status >> 2) & 0b01;
        if (bat_fault) {
            state = BATTERY_STATE_FAULT;
        } else if (!pwr_good) {
            state = BATTERY_STATE_DISCHARGING;
        }
    }

    const bool lowBat = fuel.getAlert();
    system_power_handle_state_change(g_batteryState, state, lowBat);

    if (lowBat) {
        fuel.clearAlert();
        if (s_lowBattEventEnabled) {
            s_lowBattEventEnabled = false;
            system_notify_event(low_battery);
        }
    }

    system_power_log_stat(status, curFault);
}

BatteryState system_power_handle_possible_fault(BatteryState from, BatteryState to) {
    if (from == BATTERY_STATE_CHARGED || from == BATTERY_STATE_CHARGING) {
        system_tick_t m = millis();
        if (m - s_possibleFaultTimestamp > 1000) {
            s_possibleFaultTimestamp = m;
            s_possibleFaultCounter = 0;
        } else {
            s_possibleFaultCounter++;
            if (s_possibleFaultCounter > 5) {
                return BATTERY_STATE_FAULT;
            }
        }
    }
    return to;
}

void system_power_handle_state_change(BatteryState from, BatteryState to, bool low) {
    BatteryState toOriginal = to;

    switch (from) {
        case BATTERY_STATE_CHARGING:
        case BATTERY_STATE_CHARGED: {
            if (!low) {
                to = system_power_handle_possible_fault(from, to);
            }
            break;
        }
        default: {
            if (from == to) {
                // No state change occured
                return;
            }
        }
    }

    switch (from) {
        case BATTERY_STATE_UNKNOWN:
            break;
        case BATTERY_STATE_NOT_CHARGING: {
            if (to == BATTERY_STATE_CHARGING) {
                // Reconfigure the PMIC current limits due to possible input source change
                system_power_init_default_params();
            }
            break;
        }
        case BATTERY_STATE_CHARGING:
            break;
        case BATTERY_STATE_CHARGED:
            break;
        case BATTERY_STATE_DISCHARGING:
            break;
        case BATTERY_STATE_FAULT: {
            // When going from FAULT state to any other state, reset FuelGauge
            FuelGauge fuel;
            fuel.quickStart();

            system_power_init_default_params();
            break;
        }
    }

    switch (to) {
        case BATTERY_STATE_CHARGING: {
            // When going into CHARGING state, enable low battery event
            s_lowBattEventEnabled = true;
            break;
        }
        case BATTERY_STATE_UNKNOWN:
            break;
        case BATTERY_STATE_NOT_CHARGING:
            break;
        case BATTERY_STATE_CHARGED:
            break;
        case BATTERY_STATE_DISCHARGING:
            break;
        case BATTERY_STATE_FAULT: {
            if (from != to && to != toOriginal) {
                // A fault condition was detected by system_power_handle_possible_fault()
                PMIC power;
                // Enable watchdog that should re-enable charging in 40 seconds
                power.setWatchdog(0b01);
                // Disable charging
                power.disableCharging();
            }
            break;
        }
    }

    g_batteryState = to;

#if defined(DEBUG_BUILD) || 1
    static const char* states[] = {
        "UNKNOWN",
        "NOT_CHARGING",
        "CHARGING",
        "CHARGED",
        "DISCHARGING",
        "FAULT"
    };
    LOG(TRACE, "Battery state %s -> %s", states[from], states[to]);
#endif // defined(DEBUG_BUILD)
}

void system_power_log_stat(uint8_t stat, uint8_t fault) {
#if defined(DEBUG_BUILD) && 0
    if (LOG_ENABLED(TRACE)) {
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
        LOG(TRACE, "[ PMIC STAT ] VBUS:%d CHRG:%d DPM:%d PG:%d THERM:%d VSYS:%d", vbus_stat, chrg_stat, dpm_stat, pg_stat, therm_stat, vsys_stat);
        LOG(TRACE, "[ PMIC FAULT ] WATCHDOG:%d CHRG:%d BAT:%d NTC:%d", wd_fault, chrg_fault, bat_fault, ntc_fault);
        delay(50);
    }
#endif
}
#else
void system_power_management_sleep(bool sleep) {
}
#endif
