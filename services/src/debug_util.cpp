/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#include <cstdint>

#include "debug_util.h"

#include "system_error.h"
#include "logging.h"

#if HAL_PLATFORM_NRF52840

#include <nrf52840.h>

int set_watchpoint(const void* addr, size_t size, int type) {
    size_t numComp = (DWT->CTRL >> 28) & 0x0f;
    if (!numComp) {
        LOG(ERROR, "Watchpoints are not supported");
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    // TODO: Support more than one watchpoint
    if (DWT->FUNCTION0) {
        LOG(ERROR, "Watchpoint is already set");
        return SYSTEM_ERROR_ALREADY_EXISTS;
    }
    if (!size || (size & (size - 1)) != 0) {
        LOG(ERROR, "Size is not a power of 2");
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    // Determine the mask size
    DWT->MASK0 = 0x1f;
    uint32_t maxMaskBits = DWT->MASK0;
    uint32_t maskBits = 0;
    while (size & 1) {
        ++maskBits;
        size >>= 1;
    }
    if (maskBits > maxMaskBits) {
        LOG(ERROR, "Size is too large");
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    // Address must be aligned by size
    auto addrVal = (uintptr_t)addr;
    if ((addrVal & ((1 << maskBits) - 1)) != 0) {
        LOG(ERROR, "Address is not aligned");
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    uint32_t func = 0;
    switch (type) {
    case WATCHPOINT_TYPE_READ:
        func = 0x05;
        break;
    case WATCHPOINT_TYPE_WRITE:
        func = 0x06;
        break;
    case WATCHPOINT_TYPE_READ_WRITE:
        func = 0x07;
        break;
    default:
        LOG(ERROR, "Invalid watchpoint type");
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    DWT->COMP0 = addrVal;
    DWT->MASK0 = maskBits;
    DWT->FUNCTION0 = func; // Enables the watchpoint
    return 0;
}

void clear_watchpoint(int idx) {
    // TODO: Support more than one watchpoint
    if (idx == 0) {
        DWT->FUNCTION0 = 0;
    }
}

void breakpoint() {
    asm("BKPT");
}

#endif // HAL_PLATFORM_NRF52840
