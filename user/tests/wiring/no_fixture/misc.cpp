/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
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
#include "ringbuffer.h"

test(MISC_01_map) {
    // int map(int, int, int, int, int)
    assertEqual(map(5, 0, 10, 0, 10), 5);
    assertEqual(map(5, 0, 10, 0, 20), 10);
    assertEqual(map(5, 10, 10, 10, 10), 5); // Shouldn't cause division by zero
    // double map(double, double, double, double, double)
    assertEqual(map(5.0, 0.0, 10.0, 0.0, 10.0), 5);
    assertEqual(map(5.0, 0.0, 10.0, 0.0, 15.0), 7.5);
    assertEqual(map(5.5, 10.0, 10.0, 10.0, 10.0), 5.5); // Shouldn't cause division by zero
}

test(MISC_02_alignment) {
    // This should just build correctly
    static __attribute__((used, aligned(256))) uint8_t test[63] = {};
    assertEqual((int)test[0], 0);
}

// FIXME: operator== ?
bool dumbCompareRingBuffer(const auto& a, const auto& b) {
    return a.buffer_ == b.buffer_ &&
            a.head_ == b.head_ &&
            a.tail_ == b.tail_ &&
            a.headPending_ == b.headPending_ &&
            a.tailPending_ == b.tailPending_ &&
            a.size_ == b.size_ &&
            a.curSize_ == b.curSize_ &&
            a.full_ == b.full_;
}

test(MISC_03_ringbuffer_swap_is_sane) {
    services::RingBuffer<uint8_t> a((uint8_t*)0x12345, 10);
    services::RingBuffer<uint8_t> b((uint8_t*)0xabcde, 100);
    a.head_ = 12345;
    a.tail_ = 54321;
    a.headPending_ = 12345;
    a.tailPending_ = 54321;
    b.head_ = 0xabcde;
    b.tail_ = 0xedcba;
    b.headPending_ = 0xabcde;
    b.tailPending_ = 0xedcba;

    auto copyA = a;
    auto copyB = b;
    assertTrue(dumbCompareRingBuffer(copyA, a));
    assertTrue(dumbCompareRingBuffer(copyB, b));
    std::swap(a, b);
    assertTrue(dumbCompareRingBuffer(copyA, b));
    assertTrue(dumbCompareRingBuffer(copyB, a));
}
