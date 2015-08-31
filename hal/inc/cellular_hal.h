/*
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

#ifndef CELLULAR_HAL_H
#define	CELLULAR_HAL_H

#include <stdint.h>
#include <string.h>


typedef int cellular_result_t;

/**
 * Initialize the cellular module. Called once at startup.
 */
cellular_result_t  cellular_init(void* reserved);


cellular_result_t  cellular_on(void* reserved);

cellular_result_t  cellular_off(void* reserved);


struct CellularConnect
{
    uint16_t size;
    const char* apn;
    const char* username;
    const char* password;

};

cellular_result_t  cellular_connect(void* reserved);

cellular_result_t  cellular_join(CellularConnect* connect, void* reserved);

cellular_result_t  cellular_disconnect(void* reserved);



struct CellularDevice
{
    uint16_t size;
    char iccid[21];
    char imei[16];

    CellularDevice()
    {
        memset(this, 0, sizeof(*this));
        size = sizeof(*this);
    }
};

int cellular_device_info(CellularDevice* device, void* reserved);

#endif	/* CELLULAR_HAL_H */

