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

#ifndef CELLULAR_INTERNAL_H
#define	CELLULAR_INTERNAL_H

#include "modem/enums_hal.h"
#include "cellular_hal_constants.h"

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

/* detail functions defined for unit tests */
namespace detail {
/**
 * Function for setting the cellular network provider based on the IMSI of the SIM card inserted, broken out for unit tests
 */
CellularNetProv _cellular_imsi_to_network_provider(const char* imsi);

cellular_result_t cellular_signal_impl(CellularSignalHal* signal, cellular_signal_t* signalext, bool strengthResult, const NetStatus& status);

} // namespace detail

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

#endif	/* CELLULAR_INTERNAL_H */

