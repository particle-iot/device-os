/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "check.h"
#include "core_hal.h"
#include "interrupts_hal.h"
#include "sleep_hal.h"
#include "spark_wiring_vector.h"

using spark::Vector;

static int hal_sleep_enter_stop_mode(const hal_sleep_config_t* config) {
    Vector<uint16_t> pins;
    Vector<InterruptMode> modes;
    long seconds = 0;

    auto wakeupSource = config->wakeup_sources;
    while (wakeupSource) {
        if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            if (!pins.append(reinterpret_cast<hal_wakeup_source_gpio_t*>(wakeupSource)->pin)) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
            if (!modes.append(reinterpret_cast<hal_wakeup_source_gpio_t*>(wakeupSource)->mode)) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
        } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            seconds = reinterpret_cast<hal_wakeup_source_rtc_t*>(wakeupSource)->ms / 1000;
        }
        wakeupSource = wakeupSource->next;
    }

    return HAL_Core_Enter_Stop_Mode_Ext(pins.data(), pins.size(), modes.data(), modes.size(), seconds, nullptr);
}

static int hal_sleep_enter_hibernate_mode(const hal_sleep_config_t* config) {
    long seconds = 0;
    uint32_t flags = HAL_STANDBY_MODE_FLAG_DISABLE_WKP_PIN;

    auto wakeupSource = config->wakeup_sources;
    while (wakeupSource) {
        if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            // FIXME on Gen3 platforms
        } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            seconds = reinterpret_cast<hal_wakeup_source_rtc_t*>(wakeupSource)->ms / 1000;
        }
        wakeupSource = wakeupSource->next;
    }

    return HAL_Core_Enter_Standby_Mode(seconds, flags);
}

int hal_sleep(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeup_source, void* reserved) {
    CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);

    int ret = SYSTEM_ERROR_NOT_SUPPORTED;

    switch (config->mode) {
        case HAL_SLEEP_MODE_STOP: {
            ret = hal_sleep_enter_stop_mode(config);
            break;
        }
        case HAL_SLEEP_MODE_ULTRA_LOW_POWER: {
            break;
        }
        case HAL_SLEEP_MODE_HIBERNATE: {
            ret = hal_sleep_enter_hibernate_mode(config);
            break;
        }
        default: {
            break;
        }
    }

    return ret;
}