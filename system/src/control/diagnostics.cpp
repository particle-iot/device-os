/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include "diagnostics.h"

#if SYSTEM_CONTROL_ENABLED

#include <cstring>

#include "common.h"

#include "control/diagnostics.pb.h"

#include "hal_platform.h"
#include "panic.h"
#include "check.h"

#define PB(_name) particle_ctrl_##_name

namespace particle::control::diagnostics {

namespace {

// Maximum length of the assertion text included in the reply. The text is encoded via a callback
// field, so it is not bounded by nanopb. The panic data only stores a pointer to the string, which
// may no longer refer to a valid string if the module that panicked has been updated since
const size_t MAX_PANIC_TEXT_SIZE = 256;

} // namespace

using namespace particle::control::common;

int getLastPanicInfo(ctrl_request* req) {
    PanicData panic = {};
    panic.size = sizeof(panic);
    // Returns SYSTEM_ERROR_NOT_FOUND if no panic data is available
    CHECK(panic_get_last_panic_data(&panic, nullptr));
    PB(GetLastPanicInfoReply) pbRep = {};
    pbRep.code = panic.code;
    pbRep.pc = panic.pc;
    pbRep.lr = panic.lr;
    pbRep.extra_code = panic.extra_code;
    EncodedString pbText(&pbRep.text, panic.text, panic.text ? strnlen(panic.text, MAX_PANIC_TEXT_SIZE) : 0);
#if HAL_PLATFORM_PANIC_REGISTERS_COUNT > 0
    static_assert(HAL_PLATFORM_PANIC_REGISTERS_COUNT <= sizeof(pbRep.registers) / sizeof(pbRep.registers[0]),
            "Not enough room for the saved registers in GetLastPanicInfoReply");
    for (size_t i = 0; i < HAL_PLATFORM_PANIC_REGISTERS_COUNT; ++i) {
        pbRep.registers[i] = panic.registers[i];
    }
    pbRep.registers_count = HAL_PLATFORM_PANIC_REGISTERS_COUNT;
#endif // HAL_PLATFORM_PANIC_REGISTERS_COUNT > 0
    CHECK(encodeReplyMessage(req, PB(GetLastPanicInfoReply_fields), &pbRep));
    return 0;
}

int clearLastPanicInfo(ctrl_request* req) {
    CHECK(panic_clear_last_panic_data(nullptr));
    return 0;
}

} // namespace particle::control::diagnostics

#endif // SYSTEM_CONTROL_ENABLED
