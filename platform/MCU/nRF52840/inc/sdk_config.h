/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#ifndef NRF_SDK_CONFIG_H
#define NRF_SDK_CONFIG_H

#include "nrfx_config.h"

//==========================================================
// <e> CLOCK_ENABLED - nrf_drv_clock - CLOCK peripheral driver - legacy layer
//==========================================================
#ifndef CLOCK_ENABLED
#define CLOCK_ENABLED NRFX_CLOCK_ENABLED
#endif

// <e> POWER_ENABLED - nrf_drv_power - POWER peripheral driver - legacy layer
//==========================================================
#ifndef POWER_ENABLED
#define POWER_ENABLED NRFX_POWER_ENABLED
#endif

#ifndef CLOCK_CONFIG_LF_SRC
#define CLOCK_CONFIG_LF_SRC NRFX_CLOCK_CONFIG_LF_SRC
#endif /* CLOCK_CONFIG_LF_SRC */

#ifndef CLOCK_CONFIG_LOG_ENABLED
#define CLOCK_CONFIG_LOG_ENABLED 0
#endif /* CLOCK_CONFIG_LOG_ENABLED */

#endif /* NRF_SDK_CONFIG_H */
