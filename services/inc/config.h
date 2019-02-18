/**
 ******************************************************************************
 * @file    appender.h
 * @author  Matthew McGowan
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
#ifndef CONFIG_H_
#define CONFIG_H_

#if !defined(RELEASE_BUILD) && !defined(DEBUG_BUILD)
#warning  "Defaulting to Release Build"
#define RELEASE_BUILD
#undef  DEBUG_BUILD
#endif

#ifndef LOG_INCLUDE_SOURCE_INFO
#define LOG_INCLUDE_SOURCE_INFO 0
#endif

#define MAX_SEC_WAIT_CONNECT            8       // Number of second a TCP, spark will wait
#define MAX_FAILED_CONNECTS             2       // Number of time a connect can fail
#define DEFAULT_SEC_INACTIVITY          0
#define DEFAULT_SEC_NETOPS              20

// Number of application loop iterations after which the main thread is suspended for some short time.
// This is primarily used to workaround 100% CPU usage on the virtual device platform
#ifndef SUSPEND_APPLICATION_THREAD_LOOP_COUNT
#define SUSPEND_APPLICATION_THREAD_LOOP_COUNT 25
#endif

#endif /* CONFIG_H_ */
