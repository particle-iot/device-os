/**
 ******************************************************************************
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
 ******************************************************************************
 */


// this file is used to compile the Catch unit tests on gcc, so contains few dependencies

#include "system_task.h"
#include <algorithm>
#include "system_version.h"
#include "static_assert.h"
#include "spark_macros.h"

using std::min;

/**
 * Series 1s (5 times), 2s (5 times), 4s (5 times)...64s (5 times) then to 128s thereafter.
 * @param connection_attempts
 * @return The number of milliseconds to backoff.
 */
unsigned backoff_period(unsigned connection_attempts)
{
    if (!connection_attempts)
        return 0;
    unsigned exponent = min(7u, (connection_attempts-1)/5);
    return 1000*(1<<exponent);
}

STATIC_ASSERT(system_version_info_size, sizeof(SystemVersionInfo)==28);

int system_version_info(SystemVersionInfo* info, void* /*reserved*/)
{
    if (info)
    {
        if (info->size>=28)
        {
            info->versionNumber = SYSTEM_VERSION;
            strncpy(info->versionString, stringify(SYSTEM_VERSION_STRING), sizeof(info->versionString));
        }
    }
    return sizeof(SystemVersionInfo);
}
