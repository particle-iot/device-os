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

using namespace particle::usbd::dfu::mal;

namespace {

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
    if ((addr >= INTERNAL_FLASH_START_ADD && addr <= INTERNAL_FLASH_END_ADDR) &&
        (end >= INTERNAL_FLASH_START_ADD && end <= INTERNAL_FLASH_END_ADDR))
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

    if (hal_flash_write(addr, buf, len) != 0) {
        return 1;
    }

    return 0;
}

int InternalFlashMal::erase(uintptr_t addr, size_t len) {
    if (!validate(addr, len)) {
        return 1;
    }

    (void)len;

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
  return INTERNAL_FLASH_IF_STRING;
}

DcdMal::DcdMal()
    : dfu::DfuMal() {
}

DcdMal::~DcdMal() {
}

int DcdMal::init() {
    return 0;
}

int DcdMal::deInit() {
    return 0;
}

bool DcdMal::validate(uintptr_t addr, size_t len) {
    uintptr_t end = addr + len;
    if ((addr >= DCD_START_ADD && addr <= DCD_END_ADDR) &&
        (end >= DCD_START_ADD && end <= DCD_END_ADDR))
    {
        return true;
    }

    return false;
}

int DcdMal::read(uint8_t* buf, uintptr_t addr, size_t len) {
    if (!validate(addr, len)) {
        return 1;
    }

    return dct_read_app_data_copy(addr, buf, len);
}

int DcdMal::write(const uint8_t* buf, uintptr_t addr, size_t len) {
    if (!validate(addr, len)) {
        return 1;
    }

    return dct_write_app_data( (const void*)buf, addr, len );
}

int DcdMal::erase(uintptr_t addr, size_t len) {
    return 0;
}

int DcdMal::getStatus(detail::DfuGetStatus* status, dfu::detail::DfuseCommand cmd) {
    status->bwPollTimeout[0] = status->bwPollTimeout[1] = status->bwPollTimeout[2] = 0;
    return 0;
}

const char* DcdMal::getString() {
    return DCD_IF_STRING;
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
    if ((addr >= EXTERNAL_FLASH_START_ADD && addr <= EXTERNAL_FLASH_END_ADDR) &&
        (end >= EXTERNAL_FLASH_START_ADD && end <= EXTERNAL_FLASH_END_ADDR))
    {
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
    return EXTERNAL_FLASH_IF_STRING;
}
