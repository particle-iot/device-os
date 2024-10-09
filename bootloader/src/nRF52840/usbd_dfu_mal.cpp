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

#include "usbd_dfu_mal.h"

/* Important! Needs to be included before other headers */
#include "logging.h"
#include "flash_hal.h"
#include "exflash_hal.h"
#include "dct.h"
#include "flash_mal.h"
#include "flash_common.h"

using namespace particle::usbd::dfu::mal;

extern "C" uintptr_t platform_system_part1_flash_start;
extern "C" uintptr_t platform_system_part1_flash_start_legacy;
extern "C" uintptr_t platform_system_part1_flash_end;

namespace {

bool inServiceMode() {
    int mode = security_mode_get(nullptr);
    if (mode == MODULE_INFO_SECURITY_MODE_NONE && security_mode_is_overridden()) {
        return true;
    }
    return false;
}

volatile bool s_systemRegionProtected = false;

} /* anonymous */

InternalFlashMal::InternalFlashMal()
    : dfu::DfuMal() {
}

InternalFlashMal::~InternalFlashMal() {
}

int InternalFlashMal::init() {
    return 0;
}

int InternalFlashMal::deInit() {
    return 0;
}

bool InternalFlashMal::validate(uintptr_t addr, size_t len) {
    uintptr_t end = addr + len;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
    if ((addr >= INTERNAL_FLASH_START_ADD && addr <= INTERNAL_FLASH_END_ADDR) &&
        (end >= INTERNAL_FLASH_START_ADD && end <= INTERNAL_FLASH_END_ADDR))
#pragma GCC diagnostic pop
    {
        return true;
    }

    return false;
}

int InternalFlashMal::read(uint8_t* buf, uintptr_t addr, size_t len) {
    if (!validate(addr, len)) {
        return 1;
    }

    memcpy(buf, (void*)addr, len);
    return 0;
}

int InternalFlashMal::write(const uint8_t* buf, uintptr_t addr, size_t len) {
    if (!validate(addr, len)) {
        return 1;
    }

    if (inServiceMode() && addr >= (uintptr_t)&platform_system_part1_flash_start && addr < (uintptr_t)&platform_system_part1_flash_end) {
        // Within part1 region
        if ((addr == (uintptr_t)&platform_system_part1_flash_start || addr == (uintptr_t)&platform_system_part1_flash_start_legacy)) {
            s_systemRegionProtected = true;
            if (len < (sizeof(module_info_t) + 0x200 /* vector table */)) {
                // Deny such short writes
                return SYSTEM_ERROR_PROTECTED;
            }
            module_info_t info = {};
            if (FLASH_ModuleInfo(&info, FLASH_INTERNAL, (uintptr_t)buf, nullptr) != 0) {
                return SYSTEM_ERROR_PROTECTED;
            }
            if (info.module_version < 6000 /* 6.0.0 */) {
                // This check works for both legacy and new default locations
                return SYSTEM_ERROR_PROTECTED;
            }
            s_systemRegionProtected = false;
        } else {
            if (s_systemRegionProtected) {
                return SYSTEM_ERROR_PROTECTED;
            }
        }
        if ((addr % INTERNAL_FLASH_PAGE_SIZE) == 0) {
            if (hal_flash_erase_sector(addr, CEIL_DIV(len, INTERNAL_FLASH_PAGE_SIZE)) != 0) {
                return 1;
            }
        }
    }

    if (hal_flash_write(addr, buf, len) != 0) {
        return 1;
    }

    if (addr == USER_FIRMWARE_IMAGE_LOCATION && len >= sizeof(module_info_t)) {
        // Perform some basic validation
        if (FLASH_isUserModuleInfoValid(FLASH_INTERNAL, USER_FIRMWARE_IMAGE_LOCATION, USER_FIRMWARE_IMAGE_LOCATION)) {
            // Invalidate compat 128KB application
            if (hal_flash_erase_sector(USER_FIRMWARE_IMAGE_LOCATION_COMPAT, 1) != 0) {
                return 1;
            }
        }
    }

    return 0;
}

int InternalFlashMal::erase(uintptr_t addr, size_t len) {
    if (!validate(addr, len)) {
        return 1;
    }

    (void)len;

    if (inServiceMode() && addr >= (uintptr_t)&platform_system_part1_flash_start && addr < (uintptr_t)&platform_system_part1_flash_end) {
        // Ignore erasure for now
        return 0;
    }

    if (hal_flash_erase_sector(addr, 1) != 0) {
        return 1;
    }

    return 0;
}

int InternalFlashMal::getStatus(detail::DfuGetStatus* status, dfu::detail::DfuseCommand cmd) {
  status->bwPollTimeout[0] = status->bwPollTimeout[1] = status->bwPollTimeout[2] = 0;
  return 0;
}

const char* InternalFlashMal::getString() {
  return security_mode_get(nullptr) != MODULE_INFO_SECURITY_MODE_PROTECTED ? INTERNAL_FLASH_IF_STRING : INTERNAL_FLASH_IF_STRING_PROT;
}

DctMal::DctMal()
    : dfu::DfuMal() {
}

DctMal::~DctMal() {
}

int DctMal::init() {
    return 0;
}

int DctMal::deInit() {
    return 0;
}

bool DctMal::validate(uintptr_t addr, size_t len) {
    uintptr_t end = addr + len;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
    if ((addr >= DCT_START_ADD && addr <= DCT_END_ADDR) &&
        (end >= DCT_START_ADD && end <= DCT_END_ADDR))
#pragma GCC diagnostic pop
    {
        return true;
    }

    return false;
}

int DctMal::read(uint8_t* buf, uintptr_t addr, size_t len) {
    if (!validate(addr, len)) {
        return 1;
    }

    return dct_read_app_data_copy(addr, buf, len);
}

int DctMal::write(const uint8_t* buf, uintptr_t addr, size_t len) {
    if (!validate(addr, len)) {
        return 1;
    }

    return dct_write_app_data( (const void*)buf, addr, len );
}

int DctMal::erase(uintptr_t addr, size_t len) {
    return 0;
}

int DctMal::getStatus(detail::DfuGetStatus* status, dfu::detail::DfuseCommand cmd) {
    status->bwPollTimeout[0] = status->bwPollTimeout[1] = status->bwPollTimeout[2] = 0;
    return 0;
}

const char* DctMal::getString() {
    return security_mode_get(nullptr) != MODULE_INFO_SECURITY_MODE_PROTECTED ? DCT_IF_STRING : DCT_IF_STRING_PROT;
}

ExternalFlashMal::ExternalFlashMal()
    : dfu::DfuMal() {
}

ExternalFlashMal::~ExternalFlashMal() {
}

int ExternalFlashMal::init() {
    return 0;
}

int ExternalFlashMal::deInit() {
    return 0;
}

bool ExternalFlashMal::validate(uintptr_t addr, size_t len) {
    if (addr < offset_) {
        /* NOTE: due to a bug in dfu-util in handling 0 addresses we map
         * the offset the external flash addresses by 0x80000000
         */
        return false;
    }

    /* Remove the offset */
    addr -= offset_;

    uintptr_t end = addr + len;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
    if ((addr >= EXTERNAL_FLASH_START_ADD && addr <= EXTERNAL_FLASH_END_ADDR) &&
        (end >= EXTERNAL_FLASH_START_ADD && end <= EXTERNAL_FLASH_END_ADDR))
#pragma GCC diagnostic pop
    {
        // Deny OTA region access while in Service Mode
        if ((addr >= EXTERNAL_FLASH_OTA_ADDRESS && addr < (EXTERNAL_FLASH_OTA_ADDRESS + EXTERNAL_FLASH_OTA_LENGTH)) &&
                inServiceMode()) {
            return false;
        }
        return true;
    }

    return false;
}

int ExternalFlashMal::read(uint8_t* buf, uintptr_t addr, size_t len) {
    if (!validate(addr, len)) {
        return 1;
    }

    return hal_exflash_read(addr - offset_, buf, len);
}

int ExternalFlashMal::write(const uint8_t* buf, uintptr_t addr, size_t len) {
    if (!validate(addr, len)) {
        return 1;
    }

    return hal_exflash_write(addr - offset_, buf, len);
}

int ExternalFlashMal::erase(uintptr_t addr, size_t len) {
    if (!validate(addr, len)) {
        return 1;
    }

    (void)len;
    return hal_exflash_erase_sector(addr - offset_, 1);
}

int ExternalFlashMal::getStatus(detail::DfuGetStatus* status, dfu::detail::DfuseCommand cmd) {
    status->bwPollTimeout[0] = status->bwPollTimeout[1] = status->bwPollTimeout[2] = 0;
    return 0;
}

const char* ExternalFlashMal::getString() {
    return security_mode_get(nullptr) != MODULE_INFO_SECURITY_MODE_PROTECTED ? EXTERNAL_FLASH_IF_STRING : EXTERNAL_FLASH_IF_STRING_PROT;
}
