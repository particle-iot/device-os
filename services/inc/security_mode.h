/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

#pragma once

#include "module_info.h"
#include "system_error.h"

#include <stdint.h>

#define CHECK_SECURITY_MODE_PROTECTED() \
    ({ \
        if (security_mode_get(NULL) == MODULE_INFO_SECURITY_MODE_PROTECTED) { \
            return SYSTEM_ERROR_PROTECTED; \
        } \
    })

#define SECURITY_MODE_PROTECTED_FN(_ret, _fn, _args) \
    _ret _fn ## _protected _args; \
    _ret _fn _args

#include "storage_hal.h" // Needs SECURITY_MODE_PROTECTED_FN

typedef enum security_mode_transport {
    SECURITY_MODE_TRANSPORT_NONE = 0,
    SECURITY_MODE_TRANSPORT_USB = 1,
    SECURITY_MODE_TRANSPORT_BLE = 2
} security_mode_transport;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int security_mode_init();

int security_mode_get(void* reserved);

int security_mode_find_module_extension(hal_storage_id storageId, uintptr_t start, module_info_security_mode_ext_t* ext);
int security_mode_check_control_request(security_mode_transport transport, uint16_t id);

void security_mode_override_to_none();
void security_mode_clear_override();
bool security_mode_is_overridden();

void security_mode_notify_system_ready(); // Called in the system firmware
void security_mode_notify_system_reset(); // Called in the bootloader
void security_mode_notify_system_tick(); // ditto

#ifdef __cplusplus
}
#endif // __cplusplus
