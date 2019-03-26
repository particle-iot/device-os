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

namespace particle { namespace power {

static const uint16_t DEFAULT_INPUT_CURRENT_LIMIT = 900;
static const system_tick_t DEFAULT_FAULT_WINDOW = 1000;
static const uint32_t DEFAULT_FAULT_COUNT_THRESHOLD = HAL_PLATFORM_PMIC_BQ24195_FAULT_COUNT_THRESHOLD;
static const system_tick_t DEFAULT_FAULT_SUPPRESSION_PERIOD = 60000;
static const system_tick_t DEFAULT_QUEUE_WAIT = 1000;
static const system_tick_t DEFAULT_WATCHDOG_TIMEOUT = 60000;

class PowerManager {
public:
  static PowerManager* instance();

  void init();
  void sleep(bool s = true);

protected:
  PowerManager();

private:
  static void loop(void* arg);
  static void isrHandler();
  void update();
  void handleUpdate();
  void initDefault(bool dpdm = true);
  void handleStateChange(battery_state_t from, battery_state_t to, bool low);
  battery_state_t handlePossibleFault(battery_state_t from, battery_state_t to);
  void handlePossibleFaultLoop();
  void logStat(uint8_t stat, uint8_t fault);
  void checkWatchdog();
#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
  bool detect();
#endif // HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
  void deinit();

private:
  static volatile bool update_;
  os_thread_t thread_ = nullptr;
  os_queue_t queue_ = nullptr;
  system_tick_t faultSuppressed_ = 0;
  uint32_t faultSecondaryCounter_ = 0;
  uint32_t possibleFaultCounter_ = 0;
  system_tick_t possibleFaultTimestamp_ = 0;
  bool lowBatEnabled_ = true;
  system_tick_t chargingDisabledTimestamp_ = 0;

#if HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
  bool detect_ = false;
#endif // HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
};


} } // particle::power
