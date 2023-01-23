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

#include "application.h"
#include "test.h"

namespace {
constexpr char data1[] = "test";
constexpr char data2[] = "deadbeef";
constexpr char data3[] = "\x00\x01\x02\x03";
} // anonymous

test(01_mailbox) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::DATA).data(data1, sizeof(data1) - 1)));
    assertEqual(0, pushMailboxMsg(data2, 10000));
    assertEqual(0, pushMailboxBuffer(data3, sizeof(data3) - 1));
}


test(02_mailbox_reset) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));
    // We've waited up to 10 seconds for test runner to understand that we are going to reset
    System.reset();
}

test(03_mailbox) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::DATA).data(data1, sizeof(data1) - 1)));
    assertEqual(0, pushMailboxMsg(data2, 10000));
    assertEqual(0, pushMailboxBuffer(data3, sizeof(data3) - 1));
}

test(04_mailbox_reset_postponed) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 10000));
    // We've waited up to 10 seconds for test runner to understand that we are going to reset
    auto t = new Thread("test", [](void* param) -> os_thread_return_t {
        delay(5000);
        System.reset();
    }, nullptr);
    // This will leak, but that's fine
    assertTrue(t);
}

test(05_mailbox) {
    assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::DATA).data(data1, sizeof(data1) - 1)));
    assertEqual(0, pushMailboxMsg(data2, 10000));
    assertEqual(0, pushMailboxBuffer(data3, sizeof(data3) - 1));
}
