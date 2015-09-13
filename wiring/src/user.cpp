/**
 ******************************************************************************
 * @file    user.cpp
 * @authors Matthew McGowan
 * @date    13 February 2015
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

#include "system_user.h"
#include <stddef.h>
#include <string.h>

/**
 * Declare the following function bodies as weak. They will only be used if no
 * other strong function body is found when linking.
 */
void setup() __attribute((weak));
void loop() __attribute((weak));

/**
 * Declare weak setup/loop implementations so that they are always defined.
 */

void setup()  {

}


void loop() {

}

extern char link_global_retained_initial_values;
extern char link_global_retained_start;
extern char link_global_retained_end;

/**
 * Initializes the user region of the backup ram.
 * This is provided here so it can be called from the monolithic firmware or from
 * the dynamically linked application module.
 */
void system_initialize_user_backup_ram()
{
    size_t len = &link_global_retained_end-&link_global_retained_start;
    memcpy(&link_global_retained_start, &link_global_retained_initial_values, len);
}