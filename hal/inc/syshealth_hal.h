/**
 ******************************************************************************
 * @file    syshealth_hal.h
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    25-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

#ifndef SYSHEALTH_HAL_H
#define	SYSHEALTH_HAL_H

#include "static_assert.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum eSystemHealth_ {
  FIRST_RETRY = 1,
  SECOND_RETRY = 2,
  THIRD_RETRY = 3,
  ENTERED_SparkCoreConfig,
  ENTERED_Main,
  ENTERED_WLAN_Loop,
  ENTERED_Setup,
  ENTERED_Loop,
  RAN_Loop,
  PRESERVE_APP,
  ENTER_DFU_APP_REQUEST=0xEDFA,
  ENTER_SAFE_MODE_APP_REQUEST=0x5AFE,
  CLEARED_WATCHDOG=0xFFFF
} eSystemHealth;

#if PLATFORM_ID!=3 && PLATFORM_ID!=20
// gcc enums are at least an int wide
PARTICLE_STATIC_ASSERT(system_health_16_bits, sizeof(eSystemHealth)==2);
#endif

eSystemHealth HAL_Get_Sys_Health();
void HAL_Set_Sys_Health(eSystemHealth health);

#define GET_SYS_HEALTH() HAL_Get_Sys_Health()
#define DECLARE_SYS_HEALTH(h) HAL_Set_Sys_Health(h)

#ifdef	__cplusplus
}
#endif

#endif	/* SYSHEALTH_HAL_H */

