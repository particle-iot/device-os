/*
 ******************************************************************************
 *  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
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
 ******************************************************************************
 */

#include <string.h>
#include "cellular_hal.h"
#include "deviceid_hal.h"


int HAL_Get_Device_Identifier(const char** name, char* buf, size_t buflen, unsigned index, void* reserved)
{
    if (index!=0)
        return -1;

    if (name)
        *name = "imei+iccid";

    CellularDevice device;
    cellular_device_info(&device, NULL);

    strcpy(buf, device.imei);
    strcat(buf, "-");
    strcat(buf, device.iccid);

    return 0;
}