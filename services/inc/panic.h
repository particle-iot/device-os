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
    HardFault,
    NMIFault,
    MemManage,
    BusFault,
    UsageFault,
    InvalidLenth,
    Exit,
    OutOfHeap,
    SPIOverRun,
    AssertionFailure,
    InvalidCase,
    PureVirtualCall,
    StackOverflow,
    HeapError,
} ePanicCode;

typedef void (*PanicHook)(const ePanicCode code, const void* extraInfo);

//optional function to set a hook that replaces the core body of the panic function
void panic_set_override(const PanicHook panicHook);

//actually trigger the panic function
void panic_do(const ePanicCode code, void* extraInfo, void(*)(uint32_t));

#ifdef __cplusplus
}
#endif
