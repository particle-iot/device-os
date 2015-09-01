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
 * Power on and initialize the cellular module,
 * if USART3 not initialized, will be done on first call.
 */
cellular_result_t  cellular_on(void* reserved);

/**
 * Power off the cellular module, must be powered up and initialized first.
 */
cellular_result_t  cellular_off(void* reserved);

struct CellularConnect
{
    uint16_t size;
    const char* apn;
    const char* username;
    const char* password;

};

/**
 * Connect the cellular module to the cellular network, setup the PDP context
 * and join the internet. Must be powered up and initialized first.
 */
cellular_result_t  cellular_connect(CellularConnect* connect, void* reserved);

/**
 * Disconnect the cellular module from the internet, must be connected first.
 */
cellular_result_t  cellular_disconnect(void* reserved);

cellular_result_t  cellular_fetch_ipconfig(void* config);

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

/**
 * Retrieve cellular module info, must be initialized first.
 */
int cellular_device_info(CellularDevice* device, void* reserved);

#endif	/* CELLULAR_HAL_H */

