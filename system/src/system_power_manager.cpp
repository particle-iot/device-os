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
LOG_SOURCE_CATEGORY("sys.power");

#include "system_power.h"
#include "system_power_manager.h"
#include "spark_wiring_fuel.h"
#include "spark_wiring_power.h"
#include "concurrent_hal.h"
#include "debug.h"
#include "spark_wiring_platform.h"
#include "pinmap_hal.h"

#if (HAL_PLATFORM_PMIC_BQ24195 && HAL_PLATFORM_FUELGAUGE_MAX17043)

using namespace particle::power;

namespace {

constexpr uint8_t BQ24195_VERSION = 0x23;

constexpr system_tick_t DEFAULT_FAULT_WINDOW = 1000;
constexpr uint32_t DEFAULT_FAULT_COUNT_THRESHOLD = HAL_PLATFORM_PMIC_BQ24195_FAULT_COUNT_THRESHOLD;
constexpr system_tick_t DEFAULT_FAULT_SUPPRESSION_PERIOD = 60000;

constexpr system_tick_t DEFAULT_QUEUE_WAIT = 1000;
constexpr system_tick_t DEFAULT_WATCHDOG_TIMEOUT = 60000;

constexpr hal_power_config defaultPowerConfig = {
  .flags = 0,
  .version = 0,
  .size = sizeof(hal_power_config),
  .vin_min_voltage = DEFAULT_INPUT_VOLTAGE_LIMIT,
  .vin_max_current = DEFAULT_INPUT_CURRENT_LIMIT,
  .charge_current = DEFAULT_CHARGE_CURRENT,
  .termination_voltage = DEFAULT_TERMINATION_VOLTAGE
};

uint16_t mapInputVoltageLimit(uint16_t value) {
  uint16_t baseValue = 3880;
  // Find closest matching voltage input limit value within [3880, 5080] >= 'value'
  volatile uint16_t v = std::min(std::max(value, (uint16_t)baseValue), (uint16_t)5080) - baseValue;
  v /= 80;
  v *= 80;
  v += baseValue;
  return v;
}

uint16_t mapInputCurrentLimit(uint16_t value) {
  // Find closest matching current input limit value <= 'value'
  const uint16_t inputCurrentLimits[] = {
    100, 150, 500, 900, 1200, 1500, 2000, 3000
  };

  uint16_t prev = inputCurrentLimits[0];
  for(const auto v: inputCurrentLimits) {
    if (v > value) {
      return prev;
    }
    prev = v;
  }

  return prev;
}

template <typename T>
bool isValid(T v) {
  T zero, ff;
  memset(&zero, 0x00, sizeof(zero));
  memset(&ff, 0xff, sizeof(ff));
  if (v == zero || v == ff) {
    return false;
  }

  return true;
}

} // anonymous

volatile bool PowerManager::update_ = true;

PowerManager::PowerManager() {
  os_queue_create(&queue_, sizeof(Event), 1, nullptr);
  SPARK_ASSERT(queue_ != nullptr);
}

PowerManager* PowerManager::instance() {
  static PowerManager mng;
  return &mng;
}

void PowerManager::init() {
  os_thread_t th = nullptr;
  os_thread_create(&th, "pwr", OS_THREAD_PRIORITY_CRITICAL, &PowerManager::loop, nullptr,
#if defined(DEBUG_BUILD)
    4 * 1024);
#else
    1024);
#endif // defined(DEBUIG_BUILD)
  SPARK_ASSERT(th != nullptr);
}

void PowerManager::update() {
  update_ = true;
  Event ev = Event::Update;
  os_queue_put(queue_, (const void*)&ev, 0, nullptr);
}

void PowerManager::sleep(bool s) {
#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
  if (detect_) {
#else
  {
#endif
    FuelGauge gauge;
    // When going into sleep we do not want to exceed the default charging parameters set
    // by initDefault(), which will be reset in case we are in a DISCONNECTED state with
    // PMIC watchdog enabled. Reset to the defaults and disable watchdog before going into sleep.
    if (s) {
      // Going into sleep
      if (g_batteryState == BATTERY_STATE_DISCONNECTED) {
        initDefault();
      }
      gauge.sleep();
    } else {
      // Wake up
      initDefault();
      update();
      gauge.wakeup();
    }
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

  bool vin = config_.flags & HAL_POWER_USE_VIN_SETTINGS_WITH_USB_HOST;
#if HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
  // Workaround:
  // when Gen3 device is powered by VIN, USB peripheral gets no power supply.
  // In this case BQ24195 will wrongly assume the device is connected to a USB host.
  // This workaround is to manually increase input current limit/voltage limit same
  // as when normally detecting being powered by VIN
  // More details are in clubhouse [CH34730]
  auto usb_state = HAL_USB_Get_State(nullptr);
#endif // HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE

  if (pwr_good) {
    uint8_t vbus_stat = status >> 6;
    switch (vbus_stat) {
      case 0x01: {
#if HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
        if (vin || usb_state < HAL_USB_STATE_POWERED) {
#else
        if (vin) {
#endif // HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
          src = POWER_SOURCE_VIN;
          applyVinConfig();
        } else {
          src = POWER_SOURCE_USB_HOST;
          applyDefaultConfig(src != g_powerSource);
        }
        break;
      }
      case 0x02:
        src = POWER_SOURCE_USB_ADAPTER;
        applyDefaultConfig();
        break;
      case 0x03:
        src = POWER_SOURCE_USB_OTG;
        applyDefaultConfig();
        break;
      case 0x00:
      default:
        if ((misc & 0x80) == 0x00) {
          // Not in DPDM detection anymore
          src = POWER_SOURCE_VIN;
          applyVinConfig();
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
  self->thread_ = os_thread_current(nullptr);

  {
    // FIXME: perform these before creating the thread

    // Load configuration
    self->loadConfig();

    LOG_DEBUG(INFO, "Power Management Initializing.");
#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
    if (!self->detect()) {
      goto exit;
    }
#endif // HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL

    if (self->config_.flags & HAL_POWER_MANAGEMENT_DISABLE) {
      LOG(WARN, "Disabled by configuration");
      goto exit;
    }

    // IMPORTANT: attach the interrupt handler first
#if HAL_PLATFORM_PMIC_INT_PIN_PRESENT
    attachInterrupt(PMIC_INT, &PowerManager::isrHandler, FALLING);
#endif
    attachInterrupt(LOW_BAT_UC, &PowerManager::isrHandler, FALLING);
    self->initDefault();
    FuelGauge fuel(true);
    fuel.wakeup();
    fuel.setAlertThreshold(20); // Low Battery alert at 10% (about 3.6V)
    fuel.clearAlert(); // Ensure this is cleared, or interrupts will never occur
    LOG_DEBUG(INFO, "State of Charge: %-6.2f%%", fuel.getSoC());
    LOG_DEBUG(INFO, "Battery Voltage: %-4.2fV", fuel.getVCell());

#if HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
    HAL_USB_Set_State_Change_Callback(usbStateChangeHandler, (void*)self, nullptr);
#endif
  }

  Event ev;
  while (true) {
    int r = os_queue_take(self->queue_, &ev, DEFAULT_QUEUE_WAIT, nullptr);
    if (!r) {
      if (ev == Event::ReloadConfig) {
        self->loadConfig();
        // Do not re-run DPDM
        self->initDefault(false);
        self->update_ = true;
      }
    }
    while (self->update_) {
      self->handleUpdate();
    }
    self->handlePossibleFaultLoop();
    self->checkWatchdog();
  }

exit:
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
  power.setChargeVoltage(config_.termination_voltage);

  // Set recharge threshold to default value - 100mV
  power.setRechargeThreshold(100);
  power.setChargeCurrent(config_.charge_current);
  if (dpdm) {
    // Force-start input current limit detection
    power.enableDPDM();
  }
  // Enable charging
  power.enableCharging();

  faultSuppressed_ = 0;
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
      // Charging will be re-enabled after DEFAULT_WATCHDOG_TIMEOUT
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
  // Check if runtime detection enabled
  if (!(config_.flags & HAL_POWER_PMIC_DETECTION)) {
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
    // PMIC is most likely present
#endif // #if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
    // Return it to automatic mode if power management was not explicitly disabled
    if (!(config_.flags & HAL_POWER_MANAGEMENT_DISABLE)) {
      PMIC power(true);
      power.setWatchdog(0b01);
    }
  }

  detachInterrupt(LOW_BAT_UC);

  g_batteryState = BATTERY_STATE_UNKNOWN;
  g_powerSource = POWER_SOURCE_UNKNOWN;
}

int PowerManager::setConfig(const hal_power_config* conf) {
  int ret = hal_power_store_config(conf, nullptr);
  if (isRunning()) {
    // Power manager is already running, ask it to reload the config
    Event ev = Event::ReloadConfig;
    os_queue_put(queue_, (const void*)&ev, CONCURRENT_WAIT_FOREVER, nullptr);
  }
  return ret;
}

void PowerManager::loadConfig() {
  // Ignore errors since we are anyway sanitizing the config and setting some sane defaults
  config_.size = sizeof(config_);
  int err = hal_power_load_config(&config_, nullptr);

  // Sanitize configuration
  if (err || config_.version == 0xff || !isValid(config_.size)) {
    // Flags require special processing even if the config is not valid
    // in order to keep DCT compatibility for the HAL_POWER_PMIC_DETECTION feature flag
    uint32_t flags = config_.flags;
    config_ = defaultPowerConfig;
#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
    if (!err) {
      if (flags & HAL_POWER_PMIC_DETECTION) {
        config_.flags |= HAL_POWER_PMIC_DETECTION;
      }
    }
#else
    (void)flags;
#endif // HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
  }

  if (!isValid(config_.vin_min_voltage)) {
    config_.vin_min_voltage = defaultPowerConfig.vin_min_voltage;
  }

  if (!isValid(config_.vin_max_current)) {
    config_.vin_max_current = defaultPowerConfig.vin_max_current;
  }

  if (!isValid(config_.charge_current)) {
    config_.charge_current = defaultPowerConfig.charge_current;
  }

  if (!isValid(config_.termination_voltage)) {
    config_.termination_voltage = defaultPowerConfig.termination_voltage;
  }

  logCurrentConfig();
}

void PowerManager::applyVinConfig() {
  PMIC power;
  if (power.getInputCurrentLimit() != mapInputCurrentLimit(config_.vin_max_current)) {
    power.setInputCurrentLimit(mapInputCurrentLimit(config_.vin_max_current));
  }

  if (power.getInputVoltageLimit() != mapInputVoltageLimit(config_.vin_min_voltage)) {
    power.setInputVoltageLimit(mapInputVoltageLimit(config_.vin_min_voltage));
  }
}

void PowerManager::logCurrentConfig() {
#if defined(DEBUG_BUILD) && 0
  LOG_DEBUG(TRACE, "Power configuration:");
  LOG_DEBUG(TRACE, "VIN Vmin: %u", config_.vin_min_voltage);
  LOG_DEBUG(TRACE, "VIN Imax: %u", config_.vin_max_current);
  LOG_DEBUG(TRACE, "Ichg: %u", config_.charge_current);
  LOG_DEBUG(TRACE, "Iterm: %u", config_.termination_voltage);
#endif // defined(DEBUG_BUILD) && 0
}

void PowerManager::applyDefaultConfig(bool dpdm) {
  PMIC power;
  if (power.getInputVoltageLimit() != DEFAULT_INPUT_VOLTAGE_LIMIT) {
    power.setInputVoltageLimit(DEFAULT_INPUT_VOLTAGE_LIMIT);
  }
  if (dpdm) {
    // Force-start input current limit detection
    LOG_DEBUG(TRACE, "Re-running DPDM");
    power.enableDPDM();
  }
}

void PowerManager::usbStateChangeHandler(HAL_USB_State state, void* context) {
  PowerManager* power = (PowerManager*)context;
  power->update();
}

bool PowerManager::isRunning() const {
  return thread_ != nullptr;
}

#endif /* (HAL_PLATFORM_PMIC_BQ24195 && HAL_PLATFORM_FUELGAUGE_MAX17043) */
