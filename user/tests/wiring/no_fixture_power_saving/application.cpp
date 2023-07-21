/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#ifndef PARTICLE_TEST_RUNNER

#include "application.h"
#include "unit-test/unit-test.h"

SYSTEM_MODE(MANUAL);

// make clean all TEST=wiring/no_fixture_long_running PLATFORM=boron -s COMPILE_LTO=n program-dfu DEBUG_BUILD=y
// make clean all TEST=wiring/no_fixture_long_running PLATFORM=boron -s COMPILE_LTO=n program-dfu DEBUG_BUILD=y USE_THREADING=y
//
// Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL, {
//     { "comm", LOG_LEVEL_NONE }, // filter out comm messages
//     { "system", LOG_LEVEL_INFO } // only info level for system messages
// });

UNIT_TEST_APP();

// Enable threading if compiled with "USE_THREADING=y"
#if PLATFORM_THREADING == 1 && USE_THREADING == 1
SYSTEM_THREAD(ENABLED);
#endif

#endif // PARTICLE_TEST_RUNNER