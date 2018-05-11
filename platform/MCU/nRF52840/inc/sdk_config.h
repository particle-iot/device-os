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

#ifndef USBD_ENABLED
#define USBD_ENABLED 1
#endif

// <o> USBD_CONFIG_IRQ_PRIORITY  - Interrupt priority
 

// <i> Priorities 0,2 (nRF51) and 0,1,4,5 (nRF52) are reserved for SoftDevice
// <0=> 0 (highest) 
// <1=> 1 
// <2=> 2 
// <3=> 3 
// <4=> 4 
// <5=> 5 
// <6=> 6 
// <7=> 7 

#ifndef USBD_CONFIG_IRQ_PRIORITY
#define USBD_CONFIG_IRQ_PRIORITY 7
#endif

// <o> USBD_CONFIG_DMASCHEDULER_MODE  - USBD SMA scheduler working scheme
 
// <0=> Prioritized access 
// <1=> Round Robin 

#ifndef USBD_CONFIG_DMASCHEDULER_MODE
#define USBD_CONFIG_DMASCHEDULER_MODE 0
#endif


// <e> USBD_CONFIG_LOG_ENABLED - Enable logging in the module
//==========================================================
#ifndef USBD_CONFIG_LOG_ENABLED
#define USBD_CONFIG_LOG_ENABLED 0
#endif

#ifdef SOFTDEVICE_PRESENT

#ifndef NRF_SDH_SOC_ENABLED
#define NRF_SDH_SOC_ENABLED 1
#endif /* NRF_SDH_SOC_ENABLED */

#ifndef NRF_SDH_LOG_ENABLED
#define NRF_SDH_LOG_ENABLED 0
#endif /* NRF_SDH_LOG_ENABLED */

#ifndef NRF_SDH_SOC_LOG_ENABLED
#define NRF_SDH_SOC_LOG_ENABLED 0
#endif /* NRF_SDH_SOC_LOG_ENABLED */

#ifndef CLOCK_CONFIG_SOC_OBSERVER_PRIO
#define CLOCK_CONFIG_SOC_OBSERVER_PRIO 0
#endif /* CLOCK_CONFIG_SOC_OBSERVER_PRIO */

#ifndef NRF_SDH_SOC_OBSERVER_PRIO_LEVELS
#define NRF_SDH_SOC_OBSERVER_PRIO_LEVELS 2
#endif

#ifndef NRF_SDH_ENABLED
#define NRF_SDH_ENABLED 1
#endif

#ifndef NRF_SDH_DISPATCH_MODEL
#define NRF_SDH_DISPATCH_MODEL 2
#endif

#ifndef NRF_SDH_CLOCK_LF_SRC
#define NRF_SDH_CLOCK_LF_SRC CLOCK_CONFIG_LF_SRC
#endif

#ifndef NRF_SDH_CLOCK_LF_RC_CTIV
#define NRF_SDH_CLOCK_LF_RC_CTIV 0
#endif

#ifndef NRF_SDH_CLOCK_LF_RC_TEMP_CTIV
#define NRF_SDH_CLOCK_LF_RC_TEMP_CTIV 0
#endif

#ifndef NRF_SDH_CLOCK_LF_ACCURACY
#define NRF_SDH_CLOCK_LF_ACCURACY 7
#endif

#ifndef NRF_SDH_REQ_OBSERVER_PRIO_LEVELS
#define NRF_SDH_REQ_OBSERVER_PRIO_LEVELS 2
#endif

#ifndef NRF_SDH_STATE_OBSERVER_PRIO_LEVELS
#define NRF_SDH_STATE_OBSERVER_PRIO_LEVELS 2
#endif

#ifndef NRF_SDH_STACK_OBSERVER_PRIO_LEVELS
#define NRF_SDH_STACK_OBSERVER_PRIO_LEVELS 2
#endif

#ifndef CLOCK_CONFIG_STATE_OBSERVER_PRIO
#define CLOCK_CONFIG_STATE_OBSERVER_PRIO 0
#endif

#ifndef POWER_CONFIG_STATE_OBSERVER_PRIO
#define POWER_CONFIG_STATE_OBSERVER_PRIO 0
#endif

#ifndef RNG_CONFIG_STATE_OBSERVER_PRIO
#define RNG_CONFIG_STATE_OBSERVER_PRIO 0
#endif

#ifndef NRF_SDH_ANT_STACK_OBSERVER_PRIO
#define NRF_SDH_ANT_STACK_OBSERVER_PRIO 0
#endif

#ifndef NRF_SDH_BLE_STACK_OBSERVER_PRIO
#define NRF_SDH_BLE_STACK_OBSERVER_PRIO 0
#endif

#ifndef NRF_SDH_SOC_STACK_OBSERVER_PRIO
#define NRF_SDH_SOC_STACK_OBSERVER_PRIO 0
#endif

#ifndef BLE_ADV_SOC_OBSERVER_PRIO
#define BLE_ADV_SOC_OBSERVER_PRIO 1
#endif

#ifndef BLE_DFU_SOC_OBSERVER_PRIO
#define BLE_DFU_SOC_OBSERVER_PRIO 1
#endif

#ifndef CLOCK_CONFIG_SOC_OBSERVER_PRIO
#define CLOCK_CONFIG_SOC_OBSERVER_PRIO 0
#endif

#ifndef POWER_CONFIG_SOC_OBSERVER_PRIO
#define POWER_CONFIG_SOC_OBSERVER_PRIO 0
#endif

#ifndef CLOCK_CONFIG_IRQ_PRIORITY
#define CLOCK_CONFIG_IRQ_PRIORITY NRFX_CLOCK_CONFIG_IRQ_PRIORITY
#endif

#ifndef NRF_SECTION_ITER_ENABLED
#define NRF_SECTION_ITER_ENABLED 1
#endif

#endif /* SOFTDEVICE_PRESENT */

#endif /* NRF_SDK_CONFIG_H */
