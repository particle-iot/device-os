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

#include "exrtc_hal.h"
#include "check.h"

#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL

// FIXME: for now this is all somewhat am18x5-specific, but the API itself should be somewhat extensible

int hal_exrtc_bind(hal_exrtc_instance_t instance, const hal_exrtc_binding_t* binding, void* reserved) {
    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(binding, SYSTEM_ERROR_INVALID_ARGUMENT);

#if HAL_PLATFORM_EXTERNAL_RTC
    // TODO: could later allow to change i2c address or pin mapping, but for simplicity returning an error here
    return SYSTEM_ERROR_ALREADY_EXISTS;
#endif // HAL_PLATFORM_EXTERNAL_RTC

    return Am18x5::getInstance().bind(binding->device);
}

int hal_exrtc_get_device(hal_exrtc_instance_t instance, hal_exrtc_device_t* device, void* reserved) {
    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);

    return Am18x5::getInstance().getDevice(device);
}

int hal_exrtc_unbind(hal_exrtc_instance_t instance, void* reserved) {
    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);

#if HAL_PLATFORM_EXTERNAL_RTC
    return SYSTEM_ERROR_NOT_SUPPORTED;
#endif // HAL_PLATFORM_EXTERNAL_RTC

    return Am18x5::getInstance().clearConfig(/* unbind */ true);
}

int hal_exrtc_get_status(hal_exrtc_instance_t instance, hal_exrtc_status_t* status, void* reserved, void* reserved1) {
    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);

    return Am18x5::getInstance().getStatus(status);
}

int hal_exrtc_set_config(hal_exrtc_instance_t instance, const hal_exrtc_config_t* config, const hal_exrtc_vendor_config_t* vendor, void* reserved) {
    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);

    return Am18x5::getInstance().setConfig(config, vendor);
}

int hal_exrtc_get_config(hal_exrtc_instance_t instance, hal_exrtc_config_t* config, hal_exrtc_vendor_config_t* vendor, void* reserved) {
    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);

    return Am18x5::getInstance().getConfig(config, vendor);
}

void* hal_exrtc_event_handler_add(hal_exrtc_instance_instance instance, hal_exrtc_event_handler_t handler, void* context, void* reserved) {
    if (instance != HAL_EXRTC_INSTANCE_1) {
        return nullptr;
    }

    return Am18x5::getInstance().onEvent(handler, context);
}

#endif // HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
