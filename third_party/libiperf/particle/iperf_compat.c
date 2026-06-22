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

#include "iperf_config.h"

#include "iperf.h"
#include "iperf_api.h"

/*
 * iperf.h cannot be included from C++ (C11 atomics in struct iperf_test),
 * this helper gives the C++ wrapper access to the test 'done' flag.
 */
void iperf_particle_interrupt(struct iperf_test* test) {
    if (test) {
        test->done = 1;
    }
}
