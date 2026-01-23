/*
 * Copyright (c) 2025 Particle Industries, Inc.  All rights reserved.
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

#include "exrtc_hal.h"
#include "spark_wiring_platform.h"
#include "debug.h"

#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL

using namespace particle;

int hal_external_rtc_set_config(const hal_am18x5_config_t* conf, void* reserved) {
    return Am18x5::getInstance().setConfig(conf);
}

int hal_external_rtc_get_config(hal_am18x5_config_t* conf, void* reserved) {
    return Am18x5::getInstance().getConfig(conf);
}

int hal_external_rtc_get_id(char* buf, size_t len, void* reserved) {
    return Am18x5::getInstance().getIdString(buf, len);
}

bool hal_external_rtc_is_present(void* reserved) {
    return Am18x5::getInstance().isPresent();
}

int hal_external_rtc_sleep(const hal_am18x5_sleep_config_t* conf, void* reserved) {
    return Am18x5::getInstance().sleep(conf);
}

int hal_external_rtc_on_osc_events(uint8_t events, Am18x5OscEventHandler handler, void* context, void* reserved) {
    return Am18x5::getInstance().onOscillatorEvent(events, handler, context);
}

int hal_external_rtc_get_oscillator_source(particle::Am18x5Oscillator* source, void* reserved) {
    return Am18x5::getInstance().getOscillatorSource(source);
}

#endif // HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
