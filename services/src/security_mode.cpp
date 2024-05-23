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

#include "security_mode.h"
#include "flash_mal.h"
#include "system_error.h"
#include "check.h"
#include "scope_guard.h"
#include <algorithm>
#include "system_control.h"
#include "flash_device_hal.h"
#include "core_hal.h"

namespace {

const uint32_t SECURITY_MODE_OVERRIDE_MAGIC = 0x8cd69adfu;

module_info_security_mode sSecurityMode = MODULE_INFO_SECURITY_MODE_NONE;

} // namespace

int security_mode_find_extension(hal_storage_id storageId, uintptr_t start, module_info_security_mode_ext_t* securityModeExt) {
    module_info_t info = {};
    int flashDevId = -1;
    if (storageId == HAL_STORAGE_ID_INTERNAL_FLASH) {
        flashDevId = FLASH_INTERNAL;
    } else if (storageId == HAL_STORAGE_ID_EXTERNAL_FLASH) {
        flashDevId = FLASH_SERIAL;
    } else {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    uint32_t infoOffset = 0;
    CHECK(FLASH_ModuleInfo(&info, flashDevId, start, &infoOffset));
    (void)infoOffset;

    // NOTE: this function should only be called for validated modules, just basic sanity checking
    CHECK_TRUE(info.module_function == MODULE_FUNCTION_BOOTLOADER, SYSTEM_ERROR_BAD_DATA);
    CHECK_TRUE((uintptr_t)info.module_start_address < (uintptr_t)info.module_end_address, SYSTEM_ERROR_BAD_DATA);

    // XXX: security extension only in suffix
    module_info_suffix_base_t suffix = {};
    uintptr_t end = (uintptr_t)info.module_end_address - (uintptr_t)info.module_start_address + start;
    uintptr_t suffixStart = end - sizeof(module_info_suffix_base_t);
    CHECK(hal_storage_read(storageId, suffixStart, (uint8_t*)&suffix, sizeof(suffix)));
    CHECK_TRUE(suffix.size > sizeof(module_info_suffix_base_t) + sizeof(module_info_extension_t) * 2, SYSTEM_ERROR_NOT_FOUND);
    // There are some additional extensions in suffix
    // FIXME: there should be a common way to parse the extensions, for now there are several places where the same thing is done
    for (uintptr_t offset = end - suffix.size; offset < end;) {
        module_info_extension_t ext = {};
        CHECK(hal_storage_read(storageId, offset, (uint8_t*)&ext, sizeof(ext)));
        SCOPE_GUARD({
            offset += ext.length;
        });
        if (ext.type == MODULE_INFO_EXTENSION_SECURITY_MODE) {
            module_info_security_mode_ext_t pext = {};
            CHECK(hal_storage_read(storageId, offset, (uint8_t*)&pext, sizeof(pext)));
            if (pext.security_mode == MODULE_INFO_SECURITY_MODE_PROTECTED) {
                // The only supported value for now
                if (securityModeExt) {
                    // TODO: cert
                    memcpy(securityModeExt, &pext, std::min<size_t>(sizeof(pext), securityModeExt->ext.length));
                }
                return 0;
            }
        } else if (ext.type == MODULE_INFO_EXTENSION_END) {
            break;
        }
    }

    return SYSTEM_ERROR_NOT_FOUND;
}

int security_mode_set(module_info_security_mode mode, void* reserved) {
    if (sSecurityMode == MODULE_INFO_SECURITY_MODE_NONE) {
        sSecurityMode = mode;
        return 0;
    }
    return SYSTEM_ERROR_INVALID_STATE;
}

int security_mode_get(void* reserved) {
    return sSecurityMode;
}

int security_mode_check_request(security_mode_transport transport, uint16_t id) {
    if (security_mode_get(nullptr) != MODULE_INFO_SECURITY_MODE_PROTECTED) {
        return 0;
    }

    if (transport == SECURITY_MODE_TRANSPORT_USB || transport == SECURITY_MODE_TRANSPORT_BLE) {
        switch (id) {
            case CTRL_REQUEST_SET_PROTECTED_STATE:
            case CTRL_REQUEST_GET_PROTECTED_STATE:
            case CTRL_REQUEST_DEVICE_ID:
            case CTRL_REQUEST_APP_CUSTOM: {
                return 0;
            }
        }
    }

    if (transport == SECURITY_MODE_TRANSPORT_BLE) {
        switch (id) {
            case CTRL_REQUEST_WIFI_SCAN_NETWORKS:
            case CTRL_REQUEST_WIFI_JOIN_NEW_NETWORK:
            case CTRL_REQUEST_WIFI_CLEAR_KNOWN_NETWORKS: {
                return 0;
            }
        }
    }

    return SYSTEM_ERROR_PROTECTED;
}

void security_mode_set_override() {
    HAL_Core_Write_Backup_Register(BKP_DR_08, SECURITY_MODE_OVERRIDE_MAGIC); // Disable security
}

void security_mode_clear_override() {
    HAL_Core_Write_Backup_Register(BKP_DR_08, 0); // Use default security mode
}

bool security_mode_is_overridden() {
    return HAL_Core_Read_Backup_Register(BKP_DR_08) == SECURITY_MODE_OVERRIDE_MAGIC;
}
