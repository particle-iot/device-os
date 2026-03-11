/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

 // FIXME: pinmap definitions caused problems when nrf52840.h is included, so as a simple workaround include it here first
#include <nrf52840.h>
#include "exrtc_hal.h"
#include "exrtc_hal_internal.h"
#include "pinmap_hal.h"
#include "call_once.h"

#if HAL_PLATFORM_EXTERNAL_RTC && !HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL

hal_exrtc_binding_t* hal_exrtc_default_binding() {
    static hal_exrtc_device_t device = {};
    static hal_exrtc_config_t config = {};
    static hal_exrtc_vendor_config_am18x5_t vendor = {};
    static hal_exrtc_binding_t binding = {};
    static particle::OnceFlag onceFlag;
    particle::CallOnce(onceFlag, [&]() {
        device.size = sizeof(device);
        device.version = HAL_EXRTC_API_VERSION;
        device.type = HAL_EXRTC_TYPE_AM18X5;
        device.transport = HAL_EXRTC_TRANSPORT_I2C;
        device.i2c = {
            .interface = HAL_PLATFORM_EXTERNAL_RTC_I2C,
            .address = HAL_PLATFORM_EXTERNAL_RTC_I2C_ADDR,
            .pin_int = RTC_INT,
            .pins = {RTC_WDI, PIN_INVALID, PIN_INVALID, PIN_INVALID},
            .reserved = {0, 0, 0, 0}
        };

        config.size = sizeof(config);
        config.version = HAL_EXRTC_API_VERSION;
        config.flags = HAL_EXRTC_CONFIG_USE_AS_MAIN_RTC;
        config.caps_enable = HAL_EXRTC_CAPS_AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL; 
        config.clock_source = HAL_EXRTC_CLOCK_SOURCE_EXTERNAL;

        
        vendor.base.size = sizeof(vendor);
        vendor.base.version = HAL_EXRTC_API_VERSION;
        vendor.base.type = HAL_EXRTC_TYPE_AM18X5;
        vendor.xtal_calibration_set = true;
        vendor.xtal_calibration = HAL_PLATFORM_EXTERNAL_RTC_CAL_XT;

        binding.size = sizeof(binding);
        binding.version = HAL_EXRTC_API_VERSION;
        binding.device = &device;
        binding.config = &config;
        binding.vendor = &vendor.base;        
    });
    return &binding;
}

#endif // HAL_PLATFORM_EXTERNAL_RTC && !HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
