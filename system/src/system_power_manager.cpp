/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "logging.h"
LOG_SOURCE_CATEGORY("sys.power")

#include "system_power.h"
#include "system_power_manager.h"
#include "spark_wiring_fuel.h"
#include "spark_wiring_power.h"
#include "concurrent_hal.h"
#include "debug.h"
#include "spark_wiring_platform.h"
#include "pinmap_hal.h"
// For system flags
#include "system_update.h"

#if (HAL_PLATFORM_PMIC_BQ24195 && HAL_PLATFORM_FUELGAUGE_MAX17043)

namespace {

const uint8_t BQ24195_VERSION = 0x23;

} // anonymous

using namespace particle::power;

volatile bool PowerManager::update_ = true;

PowerManager::PowerManager() {
  os_queue_create(&queue_, sizeof(update_), 1, nullptr);
  SPARK_ASSERT(queue_ != nullptr);
}

PowerManager* PowerManager::instance() {
  static PowerManager mng;
  return &mng;
}

void PowerManager::init() {
  os_thread_create(&thread_, "pwr", OS_THREAD_PRIORITY_CRITICAL, &PowerManager::loop, nullptr,
#if defined(DEBUG_BUILD)
    4 * 1024);
#else
    1024);
#endif // defined(DEBUIG_BUILD)
  SPARK_ASSERT(thread_ != nullptr);
}

void PowerManager::update() {
  update_ = true;
  os_queue_put(queue_, (const void*)&update_, 0, nullptr);
}

void PowerManager::sleep(bool s) {
  // When going into sleep we do not want to exceed the default charging parameters set
  // by initDefault(), which will be reset in case we are in a DISCONNECTED state with
  // PMIC watchdog enabled. Reset to the defaults and disable watchdog before going into sleep.
  if (s) {
    // Going into sleep
    if (g_batteryState == BATTERY_STATE_DISCONNECTED) {
      initDefault();
    }
  } else {
    // Wake up
    initDefault();
    update();
  }
}

void PowerManager::handleUpdate() {
  if (!update_) {
    return;
  }

  update_ = false;

  PMIC power(true);
  FuelGauge fuel(true);

  // In order to read the current fault status, the host has to read REG09 two times
  // consecutively. The 1st reads fault register status from the last read and the 2nd
  // reads the current fault register status.
  const uint8_t lastFault = power.getFault();
  (void)lastFault;
  const uint8_t curFault = power.getFault();
  const uint8_t status = power.getSystemStatus();
  const uint8_t misc = power.readOpControlRegister();

  // Watchdog fault
  if ((curFault) & 0x80) {
    // Restore parameters
    initDefault();
  }

  battery_state_t state = BATTERY_STATE_UNKNOWN;

  const uint8_t pwr_good = (status >> 2) & 0b01;

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
    if (bat_fault) {
      state = BATTERY_STATE_FAULT;
    } else if (!pwr_good) {
      state = BATTERY_STATE_DISCHARGING;
    }
  }

  if (g_batteryState == BATTERY_STATE_DISCONNECTED && state == BATTERY_STATE_NOT_CHARGING &&
      chargingDisabledTimestamp_) {
    // We are aware of the fact that charging has been disabled, stay in disconnected state
    state = BATTERY_STATE_DISCONNECTED;
  }

  const bool lowBat = fuel.getAlert();
  handleStateChange(g_batteryState, state, lowBat);

  power_source_t src = g_powerSource;
  if (pwr_good) {
    uint8_t vbus_stat = status >> 6;
    switch (vbus_stat) {
      case 0x01:
        src = POWER_SOURCE_USB_HOST;
        break;
      case 0x02:
        src = POWER_SOURCE_USB_ADAPTER;
        break;
      case 0x03:
        src = POWER_SOURCE_USB_OTG;
        break;
      case 0x00:
      default:
        if ((misc & 0x80) == 0x00) {
          // Not in DPDM detection anymore
          src = POWER_SOURCE_VIN;
          // It's not easy to detect when DPDM detection actually finishes,
          // so just check input current source register whenever we are in this state
          if (power.getInputCurrentLimit() != DEFAULT_INPUT_CURRENT_LIMIT) {
            power.setInputCurrentLimit(DEFAULT_INPUT_CURRENT_LIMIT);
          }
        }
        break;
    }
  } else {
    if (g_batteryState == BATTERY_STATE_DISCHARGING) {
      src = POWER_SOURCE_BATTERY;
    } else {
      src = POWER_SOURCE_UNKNOWN;
    }
  }

  if (g_powerSource != src) {
    g_powerSource = src;
    system_notify_event(power_source, (int)g_powerSource);
  }

  if (lowBat) {
    fuel.clearAlert();
    if (lowBatEnabled_) {
      lowBatEnabled_ = false;
      system_notify_event(low_battery);
    }
  }

  logStat(status, curFault);
}

void PowerManager::loop(void* arg) {
  PowerManager* self = PowerManager::instance();
  {
    LOG_DEBUG(INFO, "Power Management Initializing.");
#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
    if (!self->detect()) {
      goto exit;
    }
#endif // HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
    // IMPORTANT: attach the interrupt handler first
    attachInterrupt(LOW_BAT_UC, &PowerManager::isrHandler, FALLING);
    self->initDefault();
    FuelGauge fuel(true);
    fuel.wakeup();
    fuel.setAlertThreshold(20); // Low Battery alert at 10% (about 3.6V)
    fuel.clearAlert(); // Ensure this is cleared, or interrupts will never occur
    LOG_DEBUG(INFO, "State of Charge: %-6.2f%%", fuel.getSoC());
    LOG_DEBUG(INFO, "Battery Voltage: %-4.2fV", fuel.getVCell());
  }

  uint32_t tmp;
  while (true) {
    os_queue_take(self->queue_, &tmp, DEFAULT_QUEUE_WAIT, nullptr);
    while (self->update_) {
      self->handleUpdate();
    }
    self->handlePossibleFaultLoop();
    self->checkWatchdog();
  }

#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
exit:
#endif // HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
  self->deinit();
  os_thread_exit(nullptr);
}

void PowerManager::isrHandler() {
  PowerManager* self = PowerManager::instance();
  self->update();
}

void PowerManager::initDefault(bool dpdm) {
  PMIC power(true);
  power.begin();
  // Enters host-managed mode
  power.disableWatchdog();

  // Adjust charge voltage
  power.setChargeVoltage(4112);        // 4.112V termination voltage

  // Set recharge threshold to default value - 100mV
  power.setRechargeThreshold(100);
  // power.setChargeCurrent(0,0,0,0,0,0); // 512mA
  if (dpdm) {
    // Force-start input current limit detection
    power.enableDPDM();
  }
  // Enable charging
  power.enableCharging();

  faultSuppressed_ = 0;

  /* This only disables currently running detection, whenever the power source changes,
   * the DPDM detection will run again
   */
  // power.disableDPDM();

  /* Every time the power source changes, bq24195 will run current limit detection
   * and will change the limit anyway. This limit is instead adjusted in update()
   * under certain conditions.
   */
  // power.setInputCurrentLimit(900);     // 900mA
}

void PowerManager::handleStateChange(battery_state_t from, battery_state_t to, bool low) {
  switch (from) {
    case BATTERY_STATE_CHARGING:
    case BATTERY_STATE_CHARGED: {
      if (!low) {
        to = handlePossibleFault(from, to);
      }
    }
    // NOTE: fall-through
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
    case BATTERY_STATE_NOT_CHARGING:
      break;
    case BATTERY_STATE_CHARGING:
      break;
    case BATTERY_STATE_CHARGED:
      break;
    case BATTERY_STATE_DISCHARGING:
      break;
    case BATTERY_STATE_FAULT:
      break;
    case BATTERY_STATE_DISCONNECTED: {
      // When going from DISCONNECTED state to any other state quick start fuel gauge
      FuelGauge fuel;
      fuel.quickStart();

      initDefault();
    }
    break;
  }

  switch (to) {
    case BATTERY_STATE_CHARGING: {
      // When going into CHARGING state, enable low battery event
      lowBatEnabled_ = true;
      break;
    }
    case BATTERY_STATE_CHARGED:
      break;
    case BATTERY_STATE_UNKNOWN:
      break;
    case BATTERY_STATE_NOT_CHARGING:
      break;
    case BATTERY_STATE_DISCHARGING:
      break;
    case BATTERY_STATE_FAULT:
      break;
    case BATTERY_STATE_DISCONNECTED: {
      PMIC power;
      // Disable charging
      power.disableCharging();
      // Enable watchdog that should re-enable charging in 40 seconds
      // power.setWatchdog(0b01);
      chargingDisabledTimestamp_ = millis();
      break;
    }
  }

  if (from == to) {
      return;
  }

  g_batteryState = to;

  system_notify_event(battery_state, (int)g_batteryState);

#if defined(DEBUG_BUILD)
  static const char* states[] = {
    "UNKNOWN",
    "NOT_CHARGING",
    "CHARGING",
    "CHARGED",
    "DISCHARGING",
    "FAULT",
    "DISCONNECTED"
  };
  LOG_DEBUG(TRACE, "Battery state %s -> %s", states[from], states[to]);
#endif // defined(DEBUG_BUILD)
}

battery_state_t PowerManager::handlePossibleFault(battery_state_t from, battery_state_t to) {
  if (to == BATTERY_STATE_CHARGED || to == BATTERY_STATE_CHARGING) {
    system_tick_t m = millis();
    if (m - possibleFaultTimestamp_ > DEFAULT_FAULT_WINDOW) {
      possibleFaultTimestamp_ = m;
      possibleFaultCounter_ = 0;
      faultSecondaryCounter_ = 0;
    } else {
      possibleFaultCounter_++;
      if (possibleFaultCounter_ >= DEFAULT_FAULT_COUNT_THRESHOLD &&
         (faultSuppressed_ == 0 || (m - faultSuppressed_ >= DEFAULT_FAULT_SUPPRESSION_PERIOD))) {
        if (faultSecondaryCounter_ > 0) {
          faultSecondaryCounter_ = 0;
          return BATTERY_STATE_DISCONNECTED;
        } else {
          PMIC power;
          power.setRechargeThreshold(300);
          possibleFaultCounter_ = 0;
          faultSecondaryCounter_ = 1;
          possibleFaultTimestamp_ = millis();
        }
      }
    }
  }
  return to;
}

void PowerManager::handlePossibleFaultLoop() {
  if (faultSecondaryCounter_ == 1 && (millis() - possibleFaultTimestamp_ > DEFAULT_FAULT_WINDOW)) {
      PMIC power;
      power.setRechargeThreshold(100);
      faultSecondaryCounter_ = 0;
      faultSuppressed_ = millis();
  }
}

void PowerManager::checkWatchdog() {
  if (g_batteryState == BATTERY_STATE_DISCONNECTED &&
      ((millis() - chargingDisabledTimestamp_) >= DEFAULT_WATCHDOG_TIMEOUT)) {
    // Re-enable charging, do not run DPDM detection
    LOG_DEBUG(TRACE, "re-enabling charging");
    chargingDisabledTimestamp_ = 0;
    g_batteryState = BATTERY_STATE_UNKNOWN;
    initDefault(false);
  }
}

void PowerManager::logStat(uint8_t stat, uint8_t fault) {
#if defined(DEBUG_BUILD) && 0
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
  LOG_DEBUG(TRACE, "[ PMIC STAT ] VBUS:%d CHRG:%d DPM:%d PG:%d THERM:%d VSYS:%d", vbus_stat, chrg_stat, dpm_stat, pg_stat, therm_stat, vsys_stat);
  LOG_DEBUG(TRACE, "[ PMIC FAULT ] WATCHDOG:%d CHRG:%d BAT:%d NTC:%d", wd_fault, chrg_fault, bat_fault, ntc_fault);
#endif
}

#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
bool PowerManager::detect() {
  // Check if runtime detection enabled (DCT flag)
  uint8_t v;
  system_get_flag(SYSTEM_FLAG_PM_DETECTION, &v, nullptr);
  if (!v) {
    LOG_DEBUG(INFO, "Runtime PMIC/FuelGauge detection is not enabled");
    return false;
  }

  // Check that PMIC is present by reading its version
  PMIC power(true);
  power.begin();
  auto pVer = power.getVersion();
  if (pVer != BQ24195_VERSION) {
    LOG(WARN, "PMIC not present");
    return false;
  }
  LOG_DEBUG(INFO, "PMIC present, version %02x", (int)pVer);

  // Check that FuelGauge is present by reading its version
  FuelGauge fuel(true);
  fuel.wakeup();
  auto fVer = fuel.getVersion();
  if (fVer == 0x0000 || fVer == 0xffff) {
    LOG(WARN, "FuelGauge not present");
    return false;
  }
  fuel.clearAlert();
  LOG_DEBUG(INFO, "FuelGauge present, version %04x", (int)fVer);

  // FIXME: there is no reliable way to check whether PMIC interrupt line
  // is connected to LOW_BAT_UC

  detect_ = true;

  return true;
}
#endif // HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL

void PowerManager::deinit() {
  LOG(WARN, "Disabling system power manager");
#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
  if (detect_) {
#else
  {
#endif // #if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
    // PMIC is most likely present, return it to automatic mode
    PMIC power(true);
    power.setWatchdog(0b01);
  }

  detachInterrupt(LOW_BAT_UC);
}

#endif /* (HAL_PLATFORM_PMIC_BQ24195 && HAL_PLATFORM_FUELGAUGE_MAX17043) */
