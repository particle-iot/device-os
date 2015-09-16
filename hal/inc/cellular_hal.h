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
#include "wlan_hal.h"

typedef int cellular_result_t;

/**
 * Power on and initialize the cellular module,
 * if USART3 not initialized, will be done on first call.
 */
cellular_result_t  cellular_on(void* reserved);

/**
 * Power off the cellular module.
 */
cellular_result_t  cellular_off(void* reserved);

struct CellularCredentials
{
    uint16_t size;
    const char* apn = "";
    const char* username = "";
    const char* password = "";

    CellularCredentials()
    {
        size = sizeof(*this);
    }
};

/**
 * Wait for the cellular module to register on the GSM network.
 */
cellular_result_t  cellular_register(void* reserved);

/**
 * Activate the PDP context
 */
cellular_result_t  cellular_pdp_activate(CellularCredentials* connect, void* reserved);

/**
 * Deactivate the PDP context
 */
cellular_result_t  cellular_pdp_deactivate(void* reserved);

/**
 * Perform a GPRS attach.
 */
cellular_result_t  cellular_gprs_attach(CellularCredentials* connect, void* reserved);

/**
 * Perform a GPRS detach.
 */
cellular_result_t  cellular_gprs_detach(void* reserved);

/**
 * Fetch the ip configuration.
 */
cellular_result_t  cellular_fetch_ipconfig(WLanConfig* config);

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

/**
 * Set cellular connection parameters
 */
cellular_result_t cellular_credentials_set(const char* apn, const char* username, const char* password, void* reserved);

/**
 * Get cellular connection parameters
 */
CellularCredentials* cellular_credentials_get(void* reserved);

#endif	/* CELLULAR_HAL_H */

