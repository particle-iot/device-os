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

#include "system_power.h"
#include <cstdint>
#include "system_tick_hal.h"
#include "concurrent_hal.h"
#include "hal_platform.h"
#include "usb_hal.h"
#include "power_hal.h"

namespace particle { namespace power {

class PowerManager {
public:
  static PowerManager* instance();

  void init();
  void sleep(bool fuelGaugeSleep = true);
  void wakeup();
  int setConfig(const hal_power_config* conf);
  int getConfig(hal_power_config* conf);

protected:
  PowerManager();

private:
  static void loop(void* arg);
  static void isrHandler();
#if HAL_PLATFORM_SHARED_INTERRUPT
  static void isrHandlerEx(void* context);
#endif
  static void usbStateChangeHandler(HAL_USB_State state, void* context);
  void update();
  void handleCharging(bool batteryDisconnected = false);
  void handleUpdate();
  void initDefault(bool dpdm = true);
  void confirmBatteryState(battery_state_t from, battery_state_t to);
  void logStat(uint8_t stat, uint8_t fault);
  void checkWatchdog();
#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
  bool detect();
#endif // HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
  void deinit();
  void loadConfig();
  void applyVinConfig();
  void applyDefaultConfig(bool dpdm = false);
  void logCurrentConfig();
  bool isRunning() const;
  void resetBus();

  void deduceBatteryStateLoop();
  void deduceBatteryStateChargeDisabled();
  void deduceBatteryStateChargeEnabled();
  void batteryStateTransitioningTo(battery_state_t targetState, bool count = true);
  void clearIntermediateBatteryState(uint8_t state);

  static power_source_t powerSourceFromStatus(uint8_t status);

private:
  enum class Event {
    Update = 0,
    ReloadConfig = 1,
    Wakeup = 2
  };

  enum {
    STATE_CHARGING = 0x01,
    STATE_CHARGED = 0x02,
    STATE_NOT_CHARGING = 0x04,
    STATE_REPEATED_CHARGED = 0x08,
    STATE_ALL = 0xFF
  };

  static volatile bool update_;
  os_thread_t thread_ = nullptr;
  os_queue_t queue_ = nullptr;
  bool lowBatEnabled_ = true;
  system_tick_t chargingDisabledTimestamp_ = 0;
  bool fuelGaugeAwake_ = true;
#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
  bool detect_ = false;
#endif // HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
#if HAL_PLATFORM_POWER_MANAGEMENT_PMIC_WATCHDOG
  system_tick_t pmicWatchdogTimer_ = 0;
#endif // HAL_PLATFORM_POWER_MANAGEMENT_PMIC_WATCHDOG

  system_tick_t batMonitorTimeStamp_ = 0;
  system_tick_t battMonitorPeriod_ = 0;
  system_tick_t lastChargedTimeStamp_ = 0;
  system_tick_t repeatedChargedTimeStamp_ = 0;
  system_tick_t disableChargingTimeStamp_ = 0;
  uint8_t notChargingDebounceCount_ = 0;
  uint8_t chargingDebounceCount_ = 0;
  uint8_t vcellDebounceCount_ = 0;
  uint8_t chargedFaultCount_ = 0;
  uint8_t repeatedChargedCount_ = 0;
  bool possibleChargedFault_ = false;

  hal_power_config config_ = {};
};


} } // particle::power
