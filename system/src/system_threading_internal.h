/*
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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
*/

#ifndef SYSTEM_THREADING_INTERNAL_H
#define SYSTEM_THREADING_INTERNAL_H

#include "system_threading.h"
#include "system_network_internal.h"

// TODO: This macro won't be necessary when listening mode will be handled without blocking system loop
#if PLATFORM_THREADING
#define SYSTEM_THREAD_BUSY() \
        (!SYSTEM_THREAD_CURRENT() && network.listening())
#else
#define SYSTEM_THREAD_BUSY() (0)
#endif

#endif /* SYSTEM_THREADING_INTERNAL_H */
