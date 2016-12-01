/**
 ******************************************************************************
 * @file    process.cpp
 * @authors Julien Vanier
 * @date    1 December 2016
 ******************************************************************************
  Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.

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

#include "testapi.h"

#if Wiring_Process == 1

test(process_api) {
    Process proc;

    API_COMPILE(proc = Process::run("cat"));
    API_COMPILE(proc.pid());
    API_COMPILE(proc.kill(SIGKILL));
    API_COMPILE(proc.exited());
    API_COMPILE(proc.wait());
    API_COMPILE(proc.out().readString());
    API_COMPILE(proc.err().readString());
    API_COMPILE(proc.in().println("test"));
    API_COMPILE(proc.in().close());
}

#endif // Wiring_Process == 1
