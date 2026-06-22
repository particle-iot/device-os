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

#include <sys/utsname.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "deviceid_hal.h"
#include "bytes2hexbuf.h"
#include "preprocessor.h"

// Provided by build.mk from $(ARM_CPU)
#ifndef UNAME_MACHINE
#define UNAME_MACHINE "arm"
#endif

int uname(struct utsname* name) {
    memset(name, 0, sizeof(*name));
    strncpy(name->sysname, "Device OS", sizeof(name->sysname) - 1);

    uint8_t deviceId[HAL_DEVICE_ID_SIZE] = {};
    unsigned n = hal_get_device_id(deviceId, sizeof(deviceId));
    if (n) {
        bytes2hexbuf(deviceId, n, name->nodename);
    } else {
        strncpy(name->nodename, "unknown", sizeof(name->nodename) - 1);
    }

    strncpy(name->release, PP_STR(SYSTEM_VERSION_STRING), sizeof(name->release) - 1);
    snprintf(name->version, sizeof(name->version), "%s", PP_STR(PLATFORM_NAME));
    strncpy(name->machine, UNAME_MACHINE, sizeof(name->machine) - 1);
    return 0;
}
