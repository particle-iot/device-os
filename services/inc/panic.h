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

#include "hal_platform.h"
#include "program_regs.h"

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

#define PANIC_DATA_FLAG_HANDLED (0x01)

typedef void (*PanicHook)(const ePanicCode code, const void* extraInfo);

//optional function to set a hook that replaces the core body of the panic function
#if !defined(PARTICLE_USER_MODULE) || defined(PARTICLE_USE_UNSTABLE_API)
void panic_set_hook(const PanicHook panicHookFunction, void* reserved);
#endif // !defined(PARTICLE_USER_MODULE) || defined(PARTICLE_USE_UNSTABLE_API)

typedef struct PanicData {
    uint16_t size;
    uint8_t code; // ePanicCode
    uint8_t flags;
    const char* text; // e.g. assertion
    uintptr_t pc;
    uintptr_t lr;
    uintptr_t extra_code;
    uintptr_t registers[HAL_PLATFORM_PANIC_REGISTERS_COUNT];
} PanicData;

//actually trigger the panic function
void panic_(const ePanicCode code, const char* text, void* unused);
void panic_ext(const PanicData* data, void* reserved);

int panic_get_last_panic_data(PanicData* panic, void* reserved);
void panic_set_last_panic_data_handled(void* reserved);

#define PANIC_COMPAT(_code, _text, ...) ({ \
        PanicData _data = {}; \
        _data.size = sizeof(_data); \
        _data.code = _code; \
        _data.text = _text; \
        _data.lr = (uintptr_t)__builtin_return_address(0); /* XXX: __get_LR? */ \
        _data.pc = __get_PC(); \
        panic_ext(&_data, NULL); \
    })

#ifdef __cplusplus
}
#endif
