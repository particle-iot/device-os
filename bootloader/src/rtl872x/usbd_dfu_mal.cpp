/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include "flash_hal.h"
#include "dct.h"
#include "flash_mal.h"

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
    if ((addr >= DCT_START_ADD && addr <= DCT_END_ADDR) &&
        (end >= DCT_START_ADD && end <= DCT_END_ADDR))
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
    return DCT_IF_STRING;
}
