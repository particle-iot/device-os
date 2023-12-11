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

#define DEBUG_POWER 0

#if DEBUG_POWER
#define DBG_PWR(_fmt, ...) \
        do { \
            LOG(ERROR, _fmt, ##__VA_ARGS__); \
        } while (false)
#else
#define DBG_PWR(...)
#endif

using namespace particle::power;

namespace {

constexpr uint8_t BQ24195_VERSION = 0x23;

// For deducing battery state
constexpr system_tick_t BATTERY_STATE_NORMAL_CHECK_PERIOD = 10000;
constexpr system_tick_t BATTERY_STATE_CHANGE_CHECK_PERIOD = 1000;
constexpr system_tick_t BATTERY_CHARGED_MUTE_WINDOW = 1000;
constexpr system_tick_t BATTERY_DISABLE_CHARGING_SUPRESS_PERIOD = 8000; // How long after disable charging the VCELL can reliably reflect the VBAT
constexpr float BATTERY_CONNECTED_VCELL_THRESHOLD = 1.0f; // v
constexpr uint8_t BATTERY_VCELL_DEBOUNCE_COUNT = 2;
constexpr uint8_t BATTERY_CHARGING_DEBOUNCE_COUNT = 2;
constexpr uint8_t BATTERY_NOT_CHARGING_DEBOUNCE_COUNT = 2;
constexpr uint8_t BATTERY_CHARGED_FAULT_COUNT = 4;

constexpr system_tick_t DEFAULT_QUEUE_WAIT = 1000;
constexpr system_tick_t DEFAULT_WATCHDOG_TIMEOUT = 60000;
// FIXME: make sure to change setWatchdog() call to use the same interval
constexpr system_tick_t PMIC_WATCHDOG_INTERVAL = 40000; // 40s

constexpr uint16_t PMIC_NORMAL_RECHARGE_THRESHOLD = 100; // millivolts
constexpr uint16_t PMIC_FAULT_RECHARGE_THRESHOLD = 300; // millivolts

constexpr uint16_t PMIC_NORMAL_TERM_CHARGE_CURRENT = 256; // mA
constexpr uint16_t PMIC_FAULT_TERM_CHARGE_CURRENT = 128; // mA
constexpr system_tick_t BATTERY_REPEATED_CHARGED_WINDOW = 5000; // ms
constexpr uint8_t BATTERY_REPEATED_CHARGED_COUNT = 2;

constexpr hal_power_config defaultPowerConfig = {
  .flags = 0,
  .version = 0,
  .size = sizeof(hal_power_config),
  .vin_min_voltage = DEFAULT_INPUT_VOLTAGE_LIMIT,
  .vin_max_current = DEFAULT_INPUT_CURRENT_LIMIT,
  .charge_current = DEFAULT_CHARGE_CURRENT,
  .termination_voltage = DEFAULT_TERMINATION_VOLTAGE,
  .soc_bits = DEFAULT_SOC_18_BIT_PRECISION,
  .reserved2 = 0,
  .reserved3 = {0}
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
  size_t stack_size = HAL_PLATFORM_POWER_MANAGEMENT_STACK_SIZE;

  #if defined(DEBUG_BUILD) || DEBUG_POWER
    stack_size = 4 * 1024;
  #endif // defined(DEBUG_BUILD)

  os_thread_create(&th, "pwr", OS_THREAD_PRIORITY_CRITICAL, &PowerManager::loop, nullptr, stack_size);
  SPARK_ASSERT(th != nullptr);
}

void PowerManager::update() {
  update_ = true;
  Event ev = Event::Update;
  os_queue_put(queue_, (const void*)&ev, 0, nullptr);
}

void PowerManager::sleep(bool fuelGaugeSleep) {
#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
  if (detect_ && isRunning()) {
#else
  if (isRunning()) {
#endif
    // When going into sleep we do not want to exceed the default charging parameters set
    // by initDefault(), which will be reset in case we are in a DISCONNECTED state with
    // PMIC watchdog enabled. Reset to the defaults and disable watchdog before going into sleep.
    if (g_batteryState == BATTERY_STATE_DISCONNECTED) {
      initDefault(false);
    }
#if HAL_PLATFORM_POWER_MANAGEMENT_PMIC_WATCHDOG
    PMIC power;
    // XXX:
    power.disableWatchdog();
#endif // HAL_PLATFORM_POWER_MANAGEMENT_PMIC_WATCHDOG
    if (fuelGaugeSleep && fuelGaugeAwake_) {
      FuelGauge gauge;
      gauge.sleep();
      fuelGaugeAwake_ = false;
    } else {
      fuelGaugeAwake_ = true;
    }
  }
}

void PowerManager::wakeup() {
#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
  if (detect_ && isRunning()) {
#else
  if (isRunning()) {
#endif
    Event ev = Event::Wakeup;
    os_queue_put(queue_, (const void*)&ev, CONCURRENT_WAIT_FOREVER, nullptr);
  }
}

void PowerManager::handleCharging(bool batteryDisconnected) {
  PMIC power(true);

  if ((config_.flags & HAL_POWER_CHARGE_STATE_DISABLE) || batteryDisconnected) {
    if (power.isChargingEnabled()) {
      clearIntermediateBatteryState(STATE_ALL);
      DBG_PWR("Disabled charging.");
      power.disableCharging();
      power.disableSafetyTimer();
      disableChargingTimeStamp_ = millis();
      // Request to shorten the monitor period
      battMonitorPeriod_ = BATTERY_STATE_CHANGE_CHECK_PERIOD;
    }
    return;
  }

  if (!power.isChargingEnabled()) {
    clearIntermediateBatteryState(STATE_ALL);
    DBG_PWR("Enable charging.");
    power.enableCharging();
    power.enableSafetyTimer();
    // Request to shorten the monitor period
    battMonitorPeriod_ = BATTERY_STATE_CHANGE_CHECK_PERIOD;
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
  volatile uint8_t lastFault = power.getFault();
  const uint8_t curFault = power.getFault() | lastFault;

  // Watchdog fault or buck converter got disabled
  if ((curFault & 0x80 /*watchdog fault*/) || !power.isBuckEnabled()) {
    // Restore parameters
    initDefault();
  } else {
    // It is called in loop
    // handleCharging();
  }

  const uint8_t status = power.getSystemStatus();
  const uint8_t pwr_good = (status >> 2) & 0b01;

  // Deduce current battery state
  const uint8_t chrg_stat = (status >> 4) & 0b11;
  if (chrg_stat) {
    if (power.isChargingEnabled()) {
      // Charging or charged
      if (chrg_stat == 0b11) {
        batteryStateTransitioningTo(BATTERY_STATE_CHARGED);
      } else {
        // We might receive continuous interrupt. Counting the charging state may lead to fake charging state.
        batteryStateTransitioningTo(BATTERY_STATE_CHARGING, false);
      }
    }
    // Else charging is disabled, the register is just not updated, do nothing
  } else {
    // Now we need to deduce whether it is NOT_CHARGING, DISCHARGING, or in a FAULT state
    // const uint8_t chrg_fault = (curFault >> 4) & 0b11;
    const uint8_t bat_fault = (curFault >> 3) & 0b01;
    // const uint8_t ntc_fault = curFault & 0b111;
    if (bat_fault) {
      confirmBatteryState(g_batteryState, BATTERY_STATE_FAULT);
    } else if (!pwr_good) {
      confirmBatteryState(g_batteryState, BATTERY_STATE_DISCHARGING);
    } else {
      // We might receive continuous interrupt. Counting the not charging state may lead to fake not charging state.
      batteryStateTransitioningTo(BATTERY_STATE_NOT_CHARGING, false);
    }
  }

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

  // IMPORTANT: we cannot make a decision until DPDM detection finishes
  // we also cannot modify input current limit while DPDM is running,
  // as that will cause a race condition and we might be left with 100mA
  // ILIM after it finishes.
  if (pwr_good && !power.isInDPDM()) {
    switch (powerSourceFromStatus(status)) {
      case POWER_SOURCE_USB_HOST: {
#if HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
        if (vin || usb_state < HAL_USB_STATE_POWERED) {
#else
        if (vin) {
#endif // HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
          src = POWER_SOURCE_VIN;
          applyVinConfig();
        } else {
          src = POWER_SOURCE_USB_HOST;
          // XXX: only performing DPDM detection if we switch from another type of
          // power supply. POWER_SOURCE_UNKNOWN may only happen on boot,
          // and that case is already properly handled in initDefault()
          applyDefaultConfig(src != g_powerSource && g_powerSource != POWER_SOURCE_UNKNOWN);
        }
        break;
      }
      case POWER_SOURCE_USB_ADAPTER:
        src = POWER_SOURCE_USB_ADAPTER;
        applyDefaultConfig();
        break;
      case POWER_SOURCE_USB_OTG:
        src = POWER_SOURCE_USB_OTG;
        applyDefaultConfig();
        break;
      case POWER_SOURCE_VIN:
      default:
        src = POWER_SOURCE_VIN;
        applyVinConfig();
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

  const bool lowBat = fuel.getAlert();
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

    if (self->config_.flags & HAL_POWER_MANAGEMENT_DISABLE) {
      LOG(WARN, "Disabled by configuration");
      goto exit;
    }

    LOG_DEBUG(INFO, "Power Management Initializing.");
#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
    if (!self->detect()) {
      goto exit;
    }
#else
    self->resetBus();
#endif // HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL

    // IMPORTANT: attach the interrupt handler first
#if HAL_PLATFORM_PMIC_INT_PIN_PRESENT
    hal_gpio_mode(PMIC_INT, INPUT_PULLUP);
#if HAL_PLATFORM_SHARED_INTERRUPT
    hal_interrupt_extra_configuration_t extra = {};
    extra.version = HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION;
    extra.appendHandler = 1;
    extra.chainPriority = 0x0; // Highest priority
    hal_interrupt_attach(PMIC_INT, &isrHandlerEx, nullptr, FALLING, &extra);
#else
    attachInterrupt(PMIC_INT, &PowerManager::isrHandler, FALLING);
#endif // HAL_PLATFORM_SHARED_INTERRUPT
#endif // HAL_PLATFORM_PMIC_INT_PIN_PRESENT
    hal_pin_t pmicIntPin = LOW_BAT_UC;
#if PLATFORM_ID == PLATFORM_MSOM
    uint32_t revision = 0xFFFFFFFF;
    hal_get_device_hw_version(&revision, nullptr);
    if (revision == 1) {
      pmicIntPin = LOW_BAT_DEPRECATED;
    }
#endif

    hal_gpio_mode(pmicIntPin, INPUT_PULLUP);
    attachInterrupt(pmicIntPin, &PowerManager::isrHandler, FALLING);
    PMIC power(true);
    power.begin();
    self->initDefault();
    // Clear old fault register state
    power.getFault();
    power.getFault();
    FuelGauge fuel(true);
    fuel.wakeup();
    if ((self->config_.flags & HAL_POWER_CHARGE_STATE_DISABLE) == 0) {
      fuel.setAlertThreshold(20); // Low Battery alert at 10% (about 3.6V)
    }
    fuel.clearAlert(); // Ensure this is cleared, or interrupts will never occur
    LOG_DEBUG(INFO, "State of Charge: %-6.2f%%", fuel.getSoC());
    LOG_DEBUG(INFO, "Battery Voltage: %-4.2fV", fuel.getVCell());

#if HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE
    HAL_USB_Set_State_Change_Callback(usbStateChangeHandler, (void*)self, nullptr);
#endif

    self->battMonitorPeriod_ = BATTERY_STATE_NORMAL_CHECK_PERIOD;
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
      } else if (ev == Event::Wakeup) {
        self->initDefault();
        if (!self->fuelGaugeAwake_) {
          FuelGauge fuel(true);
          fuel.wakeup();
          self->fuelGaugeAwake_ = true;
          HAL_Delay_Milliseconds(500);
        }
        self->handleUpdate();
      }
    }
    while (self->update_) {
      DBG_PWR("self->update_");
      self->handleUpdate();
    }
    self->checkWatchdog();
    self->deduceBatteryStateLoop();
  }

exit:
  self->deinit();
  os_thread_exit(nullptr);
}

void PowerManager::isrHandler() {
  PowerManager* self = PowerManager::instance();
  self->update();
}

#if HAL_PLATFORM_SHARED_INTERRUPT
void PowerManager::isrHandlerEx(void* context) {
    isrHandler();
}
#endif

void PowerManager::initDefault(bool dpdm) {
  PMIC power(true);
  power.begin();

#if HAL_PLATFORM_POWER_MANAGEMENT_PMIC_WATCHDOG
  // We keep the watchdog running at 40s and make sure to kick it periodically
  // FIXME: Make sure to adjust PMIC_WATCHDOG_INTERVAL accordingly
  power.resetWatchdog();
  power.setWatchdog(0b01);
#else
  // Enters host-managed mode
  power.disableWatchdog();
#endif // HAL_PLATFORM_POWER_MANAGEMENT_PMIC_WATCHDOG

  // Adjust charge voltage
  power.setChargeVoltage(config_.termination_voltage);

  // Set recharge threshold to default value - 100mV
  DBG_PWR("initDefault, recharge threshold: %dmV, term current: %dmA", PMIC_NORMAL_RECHARGE_THRESHOLD, PMIC_NORMAL_TERM_CHARGE_CURRENT);
  power.setRechargeThreshold(PMIC_NORMAL_RECHARGE_THRESHOLD);
  power.setChargeCurrent(config_.charge_current);
  power.setTermChargeCurrent(PMIC_NORMAL_TERM_CHARGE_CURRENT);

  // Enable or disable charging
  if (g_batteryState != BATTERY_STATE_UNKNOWN) {
    handleCharging((g_batteryState == BATTERY_STATE_DISCONNECTED) ? true : false);
  } else {
    handleCharging(false); // We assume that the battery is connected
  }

  // Just in case make sure to enable BATFET
  if (!power.isBATFETEnabled()) {
    power.enableBATFET();
  }

  // Enable buck converter
  bool inDpDm = power.isInDPDM();
  if (!power.isBuckEnabled()) {
    power.enableBuck();
    // Make sure we restart DPDM as otherwise we may get stuck
    // with an invalid ILIM due to reading back and writing
    // a transient value by modifying the same register with enableBuck()
    if (inDpDm) {
      dpdm = true;
    }
  }

  auto powerSource = powerSourceFromStatus(power.getSystemStatus());
  bool vinWithValidLimit = (power.getInputCurrentLimit() > 100) &&
      ((powerSource == POWER_SOURCE_VIN) ||
      ((config_.flags & HAL_POWER_USE_VIN_SETTINGS_WITH_USB_HOST) && powerSource == POWER_SOURCE_USB_HOST));

  if (dpdm) {
    if (inDpDm || !vinWithValidLimit) {
      // Force-start input current limit detection
      // only if we modified REG00 while in DPDM or we are not powered by VIN
      // and we have a non-100mA limit set
      // NOTE: we have to wait for current one to finish
      auto start = HAL_Timer_Get_Milli_Seconds();
      while (power.isInDPDM() && (HAL_Timer_Get_Milli_Seconds() - start < 1000)) {
        HAL_Delay_Milliseconds(50);
      }
      power.enableDPDM();
    }
  }

  clearIntermediateBatteryState(STATE_ALL);
}

void PowerManager::confirmBatteryState(battery_state_t from, battery_state_t to) {
  // Whenever we confirm the battery state, we re-start the monitor cycle
  batMonitorTimeStamp_ = millis();
  clearIntermediateBatteryState(STATE_CHARGED | STATE_CHARGING | STATE_NOT_CHARGING);
  battMonitorPeriod_ = BATTERY_STATE_NORMAL_CHECK_PERIOD;
  
  // Charging may be temporarily enabled when battery is disconnected.
  // If we are still in the disconnected state, we need to disable charging again.
  handleCharging((to == BATTERY_STATE_DISCONNECTED) ? true : false);

  if (from == to) {
    if (to == BATTERY_STATE_CHARGED) {
      // Deduce the repeated charged state
      if (millis() - repeatedChargedTimeStamp_ < BATTERY_REPEATED_CHARGED_WINDOW) {
        repeatedChargedTimeStamp_ = millis();
        repeatedChargedCount_++;
        DBG_PWR("Repeated charged: %d", repeatedChargedCount_);
        if (repeatedChargedCount_ >= BATTERY_REPEATED_CHARGED_COUNT) {
          repeatedChargedCount_ = 0;
          DBG_PWR("Set term charge current: %dmA", PMIC_FAULT_TERM_CHARGE_CURRENT);
          // Now we probably attached a problematic battery, the term charge current
          // will persist until whenever the initDefault() is called.
          PMIC power;
          power.setTermChargeCurrent(PMIC_FAULT_TERM_CHARGE_CURRENT);
          // The PMIC might change to charging state. Shorten the check period.
          battMonitorPeriod_ = BATTERY_STATE_CHANGE_CHECK_PERIOD;
        }
      } else {
        repeatedChargedTimeStamp_ = millis();
        repeatedChargedCount_ = 0;
      }
    }
    // No state change occured
    return;
  }

  switch (from) {
    case BATTERY_STATE_DISCONNECTED: {
      // When going from DISCONNECTED state to any other state quick start fuel gauge
      FuelGauge fuel;
      fuel.quickStart();
      // It will set the recharg threshold to 100mV
      initDefault();
      break;
    }
    case BATTERY_STATE_UNKNOWN:
    case BATTERY_STATE_NOT_CHARGING:
    case BATTERY_STATE_CHARGING:
    case BATTERY_STATE_CHARGED:
    case BATTERY_STATE_DISCHARGING:
    case BATTERY_STATE_FAULT:
    default: break;
  }

  switch (to) {
    case BATTERY_STATE_CHARGING: {
      // When going into CHARGING state, enable low battery event
      lowBatEnabled_ = true;
      break;
    }
    case BATTERY_STATE_CHARGED: {
      repeatedChargedTimeStamp_ = millis();
      repeatedChargedCount_ = 0;
      DBG_PWR("Term charge current: %dmA", PMIC_NORMAL_TERM_CHARGE_CURRENT);
      // Reset the termination charge current.
      PMIC power;
      power.setTermChargeCurrent(PMIC_NORMAL_TERM_CHARGE_CURRENT);
      break;
    }
    case BATTERY_STATE_DISCONNECTED:
    case BATTERY_STATE_UNKNOWN:
    case BATTERY_STATE_NOT_CHARGING:
    case BATTERY_STATE_DISCHARGING:
    case BATTERY_STATE_FAULT:
    default: break;
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

void PowerManager::batteryStateTransitioningTo(battery_state_t targetState, bool count) {
  if (targetState == BATTERY_STATE_NOT_CHARGING) {
    // We may hit the event sequence, for example, charged/charging -> not charging when the current
    // charging state is BATTERY_STATE_NOT_CHARGING. We should clear the intermediate state.
    clearIntermediateBatteryState(STATE_CHARGED | STATE_CHARGING | STATE_REPEATED_CHARGED);
    if (g_batteryState != BATTERY_STATE_NOT_CHARGING) {
      if (count) {
        notChargingDebounceCount_++;
      } else {
        notChargingDebounceCount_ = 0;
      }
      battMonitorPeriod_ = BATTERY_STATE_CHANGE_CHECK_PERIOD;
      DBG_PWR("notChargingDebounceCount_: %d", notChargingDebounceCount_);
      if (notChargingDebounceCount_ >= BATTERY_NOT_CHARGING_DEBOUNCE_COUNT) {
        confirmBatteryState(g_batteryState, BATTERY_STATE_NOT_CHARGING);
      }
    }
  } else if (targetState == BATTERY_STATE_CHARGING) {
    // We may hit the event sequence, for example, charged/not charging -> charging when the current
    // charging state is BATTERY_STATE_CHARGING. We should clear the intermediate state.
    clearIntermediateBatteryState(STATE_CHARGED | STATE_NOT_CHARGING | STATE_REPEATED_CHARGED);
    if (g_batteryState != BATTERY_STATE_CHARGING) {
      if (count) {
        chargingDebounceCount_++;
      } else {
        chargingDebounceCount_ = 0;
      }
      battMonitorPeriod_ = BATTERY_STATE_CHANGE_CHECK_PERIOD;
      DBG_PWR("chargingDebounceCount_: %d", chargingDebounceCount_);
      if (chargingDebounceCount_ >= BATTERY_CHARGING_DEBOUNCE_COUNT) {
        confirmBatteryState(g_batteryState, BATTERY_STATE_CHARGING);
      }
    }
  } else if (targetState == BATTERY_STATE_CHARGED) {
    clearIntermediateBatteryState(STATE_CHARGING | STATE_NOT_CHARGING);
    battMonitorPeriod_ = BATTERY_STATE_CHANGE_CHECK_PERIOD;
    lastChargedTimeStamp_ = millis();
    if (count) {
      chargedFaultCount_++;
    } else {
      chargedFaultCount_ = 0;
    }
    DBG_PWR("chargedFaultCount_: %d", chargedFaultCount_);
    if (chargedFaultCount_ >= BATTERY_CHARGED_FAULT_COUNT) {
      if (possibleChargedFault_) {
        DBG_PWR("Charged fault, disconnected");
        // If we keep getting the charged event in a debouncing window, instead we assume that the battery is disconnected.
        // No need to restore the normal threshold
        confirmBatteryState(g_batteryState, BATTERY_STATE_DISCONNECTED);
      } else {
        possibleChargedFault_ = true;
        chargedFaultCount_ = 0;
        
        DBG_PWR("Set fault recharge threshold: 300mV");
        PMIC power(true);
        power.setRechargeThreshold(PMIC_FAULT_RECHARGE_THRESHOLD); // 300mV
      }
    }
  }
}

void PowerManager::clearIntermediateBatteryState(uint8_t state) {
  if (state & STATE_CHARGING) {
    chargingDebounceCount_ = 0;
  }
  if (state & STATE_CHARGED) {
    chargedFaultCount_ = 0;
    possibleChargedFault_ = false;
    lastChargedTimeStamp_ = 0;
  }
  if (state & STATE_NOT_CHARGING) {
    notChargingDebounceCount_ = 0;
  }
  if (state & STATE_REPEATED_CHARGED) {
    repeatedChargedTimeStamp_ = 0;
    repeatedChargedCount_ = 0;
  }
  vcellDebounceCount_ = 0;
}

void PowerManager::deduceBatteryStateLoop() {
  // We don't need to deduce the battery state when the battery is used as the main power supplier
  if (g_batteryState == BATTERY_STATE_DISCHARGING && battMonitorPeriod_ == BATTERY_STATE_NORMAL_CHECK_PERIOD) {
    return;
  }

  // This should be executed only when charging is enabled and we got a charged event
  if (lastChargedTimeStamp_ != 0 && (millis() - lastChargedTimeStamp_) >= BATTERY_CHARGED_MUTE_WINDOW) {
    PMIC power(true);
    if ((config_.flags & HAL_POWER_CHARGE_STATE_DISABLE) || !power.isChargingEnabled()) {
      DBG_PWR("Fault state, re-enable/disable charging!");
      // If charging is disabled as per the configuration, we shouldn't get here
      clearIntermediateBatteryState(STATE_ALL);
      battMonitorPeriod_ = BATTERY_STATE_NORMAL_CHECK_PERIOD;
      handleCharging(false); // we assume that the battery is connected
    } else {
      // We've passed the jitter window and the battery is charged indeed
      DBG_PWR("Charged!");
      const auto possibleChargedFault = possibleChargedFault_;
      confirmBatteryState(g_batteryState, BATTERY_STATE_CHARGED);
      if (possibleChargedFault) {
        // Restore normal recharge threshold
        DBG_PWR("Restore normal threshold: 100mV");
        power.setRechargeThreshold(PMIC_NORMAL_RECHARGE_THRESHOLD); // 100mV
      }
    }
    return;
  }

  if (millis() - batMonitorTimeStamp_ >= battMonitorPeriod_) {
    batMonitorTimeStamp_ = millis();
    if (config_.flags & HAL_POWER_CHARGE_STATE_DISABLE) {
      deduceBatteryStateChargeDisabled();
    } else {
      deduceBatteryStateChargeEnabled();
    }
  }
}

void PowerManager::deduceBatteryStateChargeDisabled() {
  DBG_PWR("deduceBatteryStateChargeDisabled()");
  handleCharging(); // No matter if the battery is connected or not, we're gong to disable charging

  // XXX: This is a dirty hack. Using the VCELL to deduce the battery state
  // is based on the experience that when charging is disabled, the VCELL can
  // correctly be measured.
  // When charging is disabled, the VCELL can reliably reflect the voltage on VBAT
  FuelGauge fuel(true);
  float vCell = fuel.getVCell();
  DBG_PWR("VCell: %dmV", (uint16_t)(vCell * 1000));

  // Make an assumption
  if (g_batteryState == BATTERY_STATE_UNKNOWN) {
    DBG_PWR("Battery state unknown, make an assumption:");
    if (vCell < BATTERY_CONNECTED_VCELL_THRESHOLD) {
      confirmBatteryState(g_batteryState, BATTERY_STATE_DISCONNECTED);
    } else {
      confirmBatteryState(g_batteryState, BATTERY_STATE_NOT_CHARGING);
    }
    return;
  } else if (g_batteryState == BATTERY_STATE_DISCONNECTED) {
    if (vCell > BATTERY_CONNECTED_VCELL_THRESHOLD) {
      if (vcellDebounceCount_ == 0) {
        // It's probably that the battery is attached, request to shorten the monitor period
        battMonitorPeriod_ = BATTERY_STATE_CHANGE_CHECK_PERIOD;
        DBG_PWR("Battery might be attached");
      }
      vcellDebounceCount_++;
      if (vcellDebounceCount_ >= BATTERY_VCELL_DEBOUNCE_COUNT) {
        goto connected;
      }
    } else {
      confirmBatteryState(g_batteryState, BATTERY_STATE_DISCONNECTED);
    }
    return;
  }

connected:
  PMIC power(true);
  const uint8_t status = power.getSystemStatus();
  const uint8_t pwrGood = (status >> 2) & 0b01;
  if (!pwrGood) {
    DBG_PWR("Power is not good");
    confirmBatteryState(g_batteryState, BATTERY_STATE_DISCHARGING);
  } else if (vCell < BATTERY_CONNECTED_VCELL_THRESHOLD) {
    DBG_PWR("vCell < BATTERY_CONNECTED_VCELL_THRESHOLD");
    // If battery falls below 2.0v, it's probably detached
    confirmBatteryState(g_batteryState, BATTERY_STATE_DISCONNECTED);
  } else {
    // Since charging is disabled, it must be in the not charging shate
    confirmBatteryState(g_batteryState, BATTERY_STATE_NOT_CHARGING);
  }
}

void PowerManager::deduceBatteryStateChargeEnabled() {
  DBG_PWR("deduceBatteryStateChargeEnabled()");
  PMIC power(true);

  if (g_batteryState == BATTERY_STATE_DISCONNECTED && !power.isChargingEnabled() /* MUST not enabled */) {
    if (millis() - disableChargingTimeStamp_ < BATTERY_DISABLE_CHARGING_SUPRESS_PERIOD) {
      return;
    }
    // At this point, we are still disabling charging, the VCELL can reliably reflect the voltage on VBAT
    FuelGauge fuel(true);
    float vCell = fuel.getVCell();
    DBG_PWR("VCell: %dmV", (uint16_t)(vCell * 1000));
    if (vCell < BATTERY_CONNECTED_VCELL_THRESHOLD) {
      DBG_PWR("It's still disconnected.");
      confirmBatteryState(g_batteryState, BATTERY_STATE_DISCONNECTED);
      return;
    }
    DBG_PWR("The battery might be attached");
    // Fall through
  }

  handleCharging(false); // If we reach here, we assume that the battery is connected and charging should be enabled

  const uint8_t status = power.getSystemStatus();
  const uint8_t chargeState = (status >> 4) & 0b11;
  if (chargeState == 0b11 && g_batteryState != BATTERY_STATE_CHARGED) {
    batteryStateTransitioningTo(BATTERY_STATE_CHARGED);
  } else if ((chargeState == 0b01 || chargeState == 0b10)) {
    batteryStateTransitioningTo(BATTERY_STATE_CHARGING);
  } else if (chargeState == 0b00) {
    const uint8_t pwrGood = (status >> 2) & 0b01;
    if (!pwrGood) {
      DBG_PWR("Power is not good");
      confirmBatteryState(g_batteryState, BATTERY_STATE_DISCHARGING);
    } else {
      batteryStateTransitioningTo(BATTERY_STATE_NOT_CHARGING);
    }
  } else {
    // In case that the battery is charged and somewhere just shortens the period, e.g. after waking up.
    battMonitorPeriod_ = BATTERY_STATE_NORMAL_CHECK_PERIOD;
  }
}

void PowerManager::checkWatchdog() {
#if HAL_PLATFORM_POWER_MANAGEMENT_PMIC_WATCHDOG
  if (millis() - pmicWatchdogTimer_ >= (PMIC_WATCHDOG_INTERVAL / 2)) {
    pmicWatchdogTimer_ = millis();
    PMIC power;
    power.resetWatchdog();
  }
#endif // HAL_PLATFORM_POWER_MANAGEMENT_PMIC_WATCHDOG
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
    LOG(INFO, "Runtime PMIC/FuelGauge detection is not enabled");
    return false;
  }

  resetBus();

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

void PowerManager::resetBus() {
  if (!hal_i2c_is_enabled(HAL_PLATFORM_PMIC_BQ24195_I2C, nullptr)) {
    hal_i2c_init(HAL_PLATFORM_PMIC_BQ24195_I2C, nullptr);
    hal_i2c_begin(HAL_PLATFORM_PMIC_BQ24195_I2C, I2C_MODE_MASTER, 0x00, nullptr);
    // Make sure to reset the I2C bus to avoid potentially corrupting the BQ24195 configuration if
    // we start communication with it during an ongoing write transaction (which may happen e.g. after a hard reset)
    hal_i2c_reset(HAL_PLATFORM_PMIC_BQ24195_I2C, 0, nullptr);
  }
}

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

  hal_pin_t pmicIntPin = LOW_BAT_UC;
#if PLATFORM_ID == PLATFORM_MSOM
  uint32_t revision = 0xFFFFFFFF;
  hal_get_device_hw_version(&revision, nullptr);
  if (revision == 1) {
    pmicIntPin = LOW_BAT_DEPRECATED;
  }
#endif
  detachInterrupt(pmicIntPin);

  g_batteryState = BATTERY_STATE_UNKNOWN;
  g_powerSource = POWER_SOURCE_UNKNOWN;
  thread_ = nullptr;
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

int PowerManager::getConfig(hal_power_config* conf) {
  auto destSize = conf->size;

  // Ignore errors since we are anyway sanitizing the config and setting some sane defaults
  int err = hal_power_load_config(conf, nullptr);

  // Sanitize configuration
  if (err || conf->version == 0xff || !isValid(conf->size)) {
    // Flags require special processing even if the config is not valid
    // in order to keep DCT compatibility for the HAL_POWER_PMIC_DETECTION feature flag
    uint32_t flags = conf->flags;

    // Copy the minimum overlapping structure data
    memcpy(conf, &defaultPowerConfig, std::min(destSize, defaultPowerConfig.size));

#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
    if (!err) {
      if (flags & HAL_POWER_PMIC_DETECTION) {
        conf->flags |= HAL_POWER_PMIC_DETECTION;
      }
    }
#else
    (void)flags;
#endif // HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
  }

  if (!isValid(conf->vin_min_voltage)) {
    conf->vin_min_voltage = defaultPowerConfig.vin_min_voltage;
  }

  if (!isValid(conf->vin_max_current)) {
    conf->vin_max_current = defaultPowerConfig.vin_max_current;
  }

  if (!isValid(conf->charge_current)) {
    conf->charge_current = defaultPowerConfig.charge_current;
  }

  if (!isValid(conf->termination_voltage)) {
    conf->termination_voltage = defaultPowerConfig.termination_voltage;
  }

  if (!isValid(conf->soc_bits)) {
    conf->soc_bits = defaultPowerConfig.soc_bits;
  }

  // IMPORTANT!: check size of destination config structure before writing any mode defaults

  return err;
}

void PowerManager::loadConfig() {
  // Ignore errors since we are anyway sanitizing the config and setting some sane defaults
  config_.size = sizeof(config_);
  int err = getConfig(&config_);
  (void)err;

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

power_source_t PowerManager::powerSourceFromStatus(uint8_t status) {
  auto vbusStat = status >> 6;
  switch (vbusStat) {
    case 0x01: {
      return POWER_SOURCE_USB_HOST;
    }
    case 0x02: {
      return POWER_SOURCE_USB_ADAPTER;
    }
    case 0x03: {
      return POWER_SOURCE_USB_OTG;
    }
    case 0x00: {
      return POWER_SOURCE_VIN;
    }
  }

  return POWER_SOURCE_UNKNOWN;
}

#endif /* (HAL_PLATFORM_PMIC_BQ24195 && HAL_PLATFORM_FUELGAUGE_MAX17043) */
