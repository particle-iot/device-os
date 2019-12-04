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

static int hal_sleep_enter_stop_mode(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeup_source) {
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

    int ret = HAL_Core_Enter_Stop_Mode_Ext(pins.data(), pins.size(), modes.data(), modes.size(), seconds, nullptr);
    if (ret < 0) {
        return ret;
    } 

    // IMPORTANT: It's the caller's responsibility to free this memory.
    if (ret == 0) {
        hal_wakeup_source_rtc_t* rtc_wakeup = (hal_wakeup_source_rtc_t*)malloc(sizeof(hal_wakeup_source_rtc_t));
        rtc_wakeup->base.size = sizeof(hal_wakeup_source_rtc_t);
        rtc_wakeup->base.version = HAL_SLEEP_VERSION;
        rtc_wakeup->base.type = HAL_WAKEUP_SOURCE_TYPE_RTC;
        rtc_wakeup->base.next = nullptr;
        rtc_wakeup->ms = 0;
        *wakeup_source = reinterpret_cast<hal_wakeup_source_base_t*>(rtc_wakeup);
    } else {
        hal_wakeup_source_gpio_t* gpio_wakeup = (hal_wakeup_source_gpio_t*)malloc(sizeof(hal_wakeup_source_gpio_t));
        gpio_wakeup->base.size = sizeof(hal_wakeup_source_gpio_t);
        gpio_wakeup->base.version = HAL_SLEEP_VERSION;
        gpio_wakeup->base.type = HAL_WAKEUP_SOURCE_TYPE_GPIO;
        gpio_wakeup->base.next = nullptr;
        gpio_wakeup->pin = ret;
        *wakeup_source = reinterpret_cast<hal_wakeup_source_base_t*>(gpio_wakeup);
    }
    return SYSTEM_ERROR_NONE;
}

static int hal_sleep_enter_hibernate_mode(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeup_source) {
    long seconds = 0;
    uint32_t flags = HAL_STANDBY_MODE_FLAG_DISABLE_WKP_PIN;

    auto wakeupSource = config->wakeup_sources;
    while (wakeupSource) {
        if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            // Hotfix for Gen2 platforms. As long as user specifies pins as wakeup source.
            flags &= ~HAL_STANDBY_MODE_FLAG_DISABLE_WKP_PIN;
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
            ret = hal_sleep_enter_stop_mode(config, wakeup_source);
            break;
        }
        case HAL_SLEEP_MODE_ULTRA_LOW_POWER: {
            break;
        }
        case HAL_SLEEP_MODE_HIBERNATE: {
            ret = hal_sleep_enter_hibernate_mode(config, wakeup_source);
            break;
        }
        default: {
            break;
        }
    }

    return ret;
}