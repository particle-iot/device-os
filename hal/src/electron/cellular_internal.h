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

#ifndef CELLULAR_HAL_MDM_H
#define	CELLULAR_HAL_MDM_H

#include "modem/enums_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Function for processing Set cellular data usage info, broken out for unit tests
 */
cellular_result_t _cellular_data_usage_set(CellularDataHal &data, const MDM_DataUsage &data_usage, bool ret);

/**
 * Function for processing Get cellular data usage info, broken out for unit tests
 */
cellular_result_t _cellular_data_usage_set(CellularDataHal &data, const MDM_DataUsage &data_usage, bool ret);

/**
 * Set cellular band select
 */
cellular_result_t cellular_band_select_set(MDM_BandSelect* bands, void* reserved);

/**
 * Get cellular band select
 */
cellular_result_t cellular_band_select_get(MDM_BandSelect* bands, void* reserved);

/**
 * Get cellular band available
 */
cellular_result_t cellular_band_available_get(MDM_BandSelect* bands, void* reserved);

#ifdef __cplusplus
}
#endif

#endif	/* CELLULAR_HAL_MDM_H */

