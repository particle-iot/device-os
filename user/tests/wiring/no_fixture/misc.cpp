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
