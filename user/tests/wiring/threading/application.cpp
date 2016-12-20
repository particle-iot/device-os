/**
 ******************************************************************************
 * @file    application.cpp
 * @authors mat
 * @date    21 January 2015
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "application.h"
#include "unit-test/unit-test.h"

// make clean all TEST=wiring/threading PLATFORM=electron -s COMPILE_LTO=n program-dfu DEBUG_BUILD=y
//
// Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL, {
//     { "comm", LOG_LEVEL_NONE }, // filter out comm messages
//     { "system", LOG_LEVEL_INFO } // only info level for system messages
// });

UNIT_TEST_APP();
SYSTEM_THREAD(ENABLED);

