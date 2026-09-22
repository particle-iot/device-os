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

#include "application.h"
#include "unit-test/unit-test.h"

#include "service_debug.h"

#if HAL_PLATFORM_FILESYSTEM

test(PANIC_INFO_01_expected_panic) {
    // Notify the runner about the upcoming panic and wait for the ACK, so the runner
    // expects the reboot and verifies the panic info stored by the panic handler
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::PANIC_PENDING), 20000));
    // A real assertion panic: code AssertionFailure (10) with the assertion text, pc and lr
    // pointing at this test - the same data the runner reports for unexpected panics
    SPARK_ASSERT(false);
}

test(PANIC_INFO_02_no_panic_info_left) {
    // The runner consumed and cleared the panic record after the expected panic; the
    // record must not reappear after the reboot that followed
}

#endif // HAL_PLATFORM_FILESYSTEM