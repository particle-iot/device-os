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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HardFault           = 1,
    NMIFault            = 2,
    MemManage           = 3,
    BusFault            = 4,
    UsageFault          = 5,
    InvalidLenth        = 6,
    Exit                = 7,
    OutOfHeap           = 8,
    SPIOverRun          = 9,
    AssertionFailure    = 10,
    InvalidCase         = 11,
    PureVirtualCall     = 12,
    StackOverflow       = 13,
    HeapError           = 14,
    SecureFault         = 15,
} ePanicCode;

typedef void (*PanicHook)(const ePanicCode code, const void* extraInfo);

//optional function to set a hook that replaces the core body of the panic function
#if !defined(PARTICLE_USER_MODULE) || defined(PARTICLE_USE_UNSTABLE_API)
void panic_set_hook(const PanicHook panicHookFunction, void* reserved);
#endif // !defined(PARTICLE_USER_MODULE) || defined(PARTICLE_USE_UNSTABLE_API)

//actually trigger the panic function
void panic_(const ePanicCode code, void* extraInfo, void(*)(uint32_t));

#ifdef __cplusplus
}
#endif
