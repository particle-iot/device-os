/**
 ******************************************************************************
 * @file    core_hal.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CORE_HAL_H
#define __CORE_HAL_H

/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/
/*
 * Use the JTAG IOs as standard GPIOs (D3 to D7)
 * Note that once the JTAG IOs are disabled, the connection with the host debugger
 * is lost and cannot be re-established as long as the JTAG IOs remain disabled.
 */
#ifndef USE_SWD_JTAG
#define SWD_JTAG_DISABLE
#endif

/*
 * Use Independent Watchdog to force a system reset when a software error occurs
 * During JTAG program/debug, the Watchdog counting is disabled by debug configuration
 */
#define IWDG_RESET_ENABLE

//#undef SPARK_WLAN_ENABLE

/*
 * default status, works the way it does now, no need to define explicitly
 */
//#define RGB_NOTIFICATIONS_ON
/*
 * should prevent any of the Spark notifications from ever being shown on the LED;
 * if the user never takes manual control of the LED, it should never turn on.
 */
//#define RGB_NOTIFICATIONS_OFF
/*
 * keep all of the statuses except the 'breathing cyan' - this would be a good
 * power saver while still showing when important things are happening on the Core.
 */
//#define RGB_NOTIFICATIONS_CONNECTING_ONLY

/* Exported functions --------------------------------------------------------*/

#endif  /* __CORE_HAL_H */
