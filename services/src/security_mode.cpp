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
#include "hal_platform.h"

#if PLATFORM_ID != PLATFORM_GCC && PLATFORM_ID != PLATFORM_NEWHAL

#include "flash_mal.h"
#include "system_error.h"
#include "check.h"
#include "scope_guard.h"
#include "atomic_section.h"
#include "system_control.h"
#include "flash_device_hal.h"
#include "concurrent_hal.h"
#include "ota_flash_hal_impl.h"
#include "hw_config.h"

#include <algorithm>

namespace {

volatile int sCurrentSecurityMode = MODULE_INFO_SECURITY_MODE_NONE;
int sNormalSecurityMode = MODULE_INFO_SECURITY_MODE_NONE;

#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER

volatile int sSystemTickCount = 1000;

#else

os_timer_t sTimer = nullptr;

void timerCallback(os_timer_t timer);

void stopTimer() {
    if (sTimer) {
        int r = os_timer_change(sTimer, OS_TIMER_CHANGE_STOP, false /* fromISR */, 0 /* period */, 0xffffffffu /* block */, nullptr /* reserved */);
        if (r != 0) {
            LOG_DEBUG(ERROR, "Failed to stop timer"); // Should not happen
        }
    }
}

int startTimer() {
    if (!sTimer) {
        int r = os_timer_create(&sTimer, 1000 /* period */, timerCallback, nullptr /* timer_id */, false /* one_shot */, nullptr /* reserved */);
        if (r != 0) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
    } else {
        stopTimer();
    }
    int r = os_timer_change(sTimer, OS_TIMER_CHANGE_START, false /* fromISR */, 0 /* period */, 0xffffffffu /* block */, nullptr /* reserved */);
    if (r != 0) {
        return SYSTEM_ERROR_UNKNOWN; // Should not happen
    }
    return 0;
}

void timerCallback(os_timer_t) {
    bool stop = false;
    ATOMIC_BLOCK() {
        Load_SystemFlags();
        if (system_flags.security_mode_override_value != 0xff) {
            if (system_flags.security_mode_override_timeout > 0) {
                --system_flags.security_mode_override_timeout;
            }
            if (!system_flags.security_mode_override_timeout) {
                system_flags.security_mode_override_value = 0xff;
            }
            Save_SystemFlags();
        }
        if (system_flags.security_mode_override_value == 0xff) {
            sCurrentSecurityMode = sNormalSecurityMode;
            stop = true;
        }
    }
    if (stop) {
        stopTimer();
    }
}

#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

} // namespace

int security_mode_init() {
    // Note that this function is called early on boot, before the heap is configured and C++
    // constructors are called
    int result = 0;
    int normalMode = MODULE_INFO_SECURITY_MODE_NONE;
    if (FLASH_VerifyCRC32(FLASH_INTERNAL, module_bootloader.start_address, FLASH_ModuleLength(FLASH_INTERNAL, module_bootloader.start_address))) {
        module_info_security_mode_ext_t ext = {};
        ext.ext.length = sizeof(ext);
        int r = security_mode_find_module_extension(HAL_STORAGE_ID_INTERNAL_FLASH, module_bootloader.start_address, &ext);
        if (r >= 0 && ext.security_mode == MODULE_INFO_SECURITY_MODE_PROTECTED) {
            normalMode = ext.security_mode;
        } else if (r < 0 && r != SYSTEM_ERROR_NOT_FOUND) {
            // XXX: It's not ideal that security_mode_find_module_extension() returns SYSTEM_ERROR_NOT_FOUND
            // if the bootloader is not protected as that code may also indicate a legit error that occurred
            // elsewhere down the stack
            LOG(ERROR, "Failed to parse bootloader module extensions: %d", r);
            result = r;
        }
    } else {
        LOG(ERROR, "Invalid bootloader checksum");
        result = SYSTEM_ERROR_BAD_DATA;
    }
    int currentMode = normalMode;
    Load_SystemFlags();
    if (system_flags.security_mode_override_value != 0xff) {
        if (normalMode != MODULE_INFO_SECURITY_MODE_NONE) {
            currentMode = system_flags.security_mode_override_value;
        } else {
            // Clear the override in case the bootloader was protected previously
            system_flags.security_mode_override_value = 0xff;
            Save_SystemFlags();
        }
    }
    sNormalSecurityMode = normalMode;
    sCurrentSecurityMode = currentMode;
    return result;
}

int security_mode_get(void* reserved) {
    return sCurrentSecurityMode;
}

int security_mode_find_module_extension(hal_storage_id storageId, uintptr_t start, module_info_security_mode_ext_t* securityModeExt) {
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

int security_mode_check_control_request(security_mode_transport transport, uint16_t id) {
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

void security_mode_override_to_none() {
    if (sNormalSecurityMode == MODULE_INFO_SECURITY_MODE_NONE) {
        return;
    }
    ATOMIC_BLOCK() {
        Load_SystemFlags();
        system_flags.security_mode_override_value = MODULE_INFO_SECURITY_MODE_NONE;
        system_flags.security_mode_override_reset_count = 20;
        system_flags.security_mode_override_timeout = 24 * 60 * 60; // Seconds
        Save_SystemFlags();
        sCurrentSecurityMode = MODULE_INFO_SECURITY_MODE_NONE;
    }
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    startTimer();
#endif
}

void security_mode_clear_override() {
    if (sCurrentSecurityMode == sNormalSecurityMode) {
        return;
    }
    ATOMIC_BLOCK() {
        Load_SystemFlags();
        system_flags.security_mode_override_value = 0xff;
        Save_SystemFlags();
        sCurrentSecurityMode = sNormalSecurityMode;
    }
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    stopTimer();
#endif
}

bool security_mode_is_overridden() {
    return sCurrentSecurityMode != sNormalSecurityMode;
}

#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER

void security_mode_notify_system_reset() {
    if (sCurrentSecurityMode == sNormalSecurityMode) {
        return;
    }
    ATOMIC_BLOCK() {
        Load_SystemFlags();
        if (system_flags.security_mode_override_value != 0xff) {
            if (system_flags.security_mode_override_reset_count > 0) {
                --system_flags.security_mode_override_reset_count;
            }
            if (!system_flags.security_mode_override_reset_count) {
                system_flags.security_mode_override_value = 0xff;
            }
            Save_SystemFlags();
        }
        if (system_flags.security_mode_override_value == 0xff) {
            sCurrentSecurityMode = sNormalSecurityMode;
        }
    }
}

void security_mode_notify_system_tick() {
    if (sCurrentSecurityMode == sNormalSecurityMode) {
        return;
    }
    if (sSystemTickCount > 0) {
        --sSystemTickCount;
    }
    if (!sSystemTickCount) {
        ATOMIC_BLOCK() {
            Load_SystemFlags();
            if (system_flags.security_mode_override_value != 0xff) {
                if (system_flags.security_mode_override_timeout > 0) {
                    --system_flags.security_mode_override_timeout;
                }
                if (!system_flags.security_mode_override_timeout) {
                    system_flags.security_mode_override_value = 0xff;
                }
                Save_SystemFlags();
            }
            if (system_flags.security_mode_override_value == 0xff) {
                sCurrentSecurityMode = sNormalSecurityMode;
            }
        }
        sSystemTickCount = 1000;
    }
}

#else

void security_mode_notify_system_ready() {
    if (sCurrentSecurityMode == sNormalSecurityMode) {
        return;
    }
    int r = startTimer();
    if (r < 0) {
        LOG(ERROR, "Failed to start timer: %d", r);
    }
}

#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

#else // PLATFORM_ID == PLATFORM_GCC || PLATFORM_ID == PLATFORM_NEWHAL

int security_mode_get(void* reserved) {
    return MODULE_INFO_SECURITY_MODE_NONE;
}

bool security_mode_is_overridden() {
    return false;
}

void security_mode_notify_system_ready() {
}

int security_mode_find_module_extension(hal_storage_id storageId, uintptr_t start, module_info_security_mode_ext_t* securityModeExt) {
    return SYSTEM_ERROR_NOT_FOUND;
}

#endif // PLATFORM_ID == PLATFORM_GCC || PLATFORM_ID == PLATFORM_NEWHAL
