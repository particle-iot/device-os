/**
 ******************************************************************************
 * @file    usbd_usr.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    21-Oct-2014
 * @brief   This file includes the user application layer
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  Copyright 2012 STMicroelectronics
  http://www.st.com/software_license_agreement_liberty_v2

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

/* Includes ------------------------------------------------------------------*/
#include "usbd_usr.h"
#include "usbd_ioreq.h"

USBD_Usr_cb_TypeDef USR_cb =
{
        USBD_USR_Init,
        USBD_USR_DeviceReset,
        USBD_USR_DeviceConfigured,
        USBD_USR_DeviceSuspended,
        USBD_USR_DeviceResumed,

        USBD_USR_DeviceConnected,
        USBD_USR_DeviceDisconnected,
};

/**
 * @brief  USBD_USR_Init
 * @param  None
 * @retval None
 */
void USBD_USR_Init(void)
{
    //USB Device Init
}

/**
 * @brief  USBD_USR_DeviceReset
 * @param  speed : device speed
 * @retval None
 */
void USBD_USR_DeviceReset(uint8_t speed )
{
    //USB Device Reset
}


/**
 * @brief  USBD_USR_DeviceConfigured
 * @param  None
 * @retval Staus
 */
void USBD_USR_DeviceConfigured (void)
{
    //USB Device Configured
}
/**
 * @brief  USBD_USR_DeviceSuspended
 * @param  None
 * @retval None
 */
void USBD_USR_DeviceSuspended(void)
{
    //USB Device in Suspend Mode
}


/**
 * @brief  USBD_USR_DeviceResumed
 * @param  None
 * @retval None
 */
void USBD_USR_DeviceResumed(void)
{
    //USB Device in Idle Mode
}

/**
 * @brief  USBD_USR_DeviceConnected
 * @param  None
 * @retval Staus
 */
void USBD_USR_DeviceConnected (void)
{
    //USB Device Connected
}


/**
 * @brief  USBD_USR_DeviceDisonnected
 * @param  None
 * @retval Staus
 */
void USBD_USR_DeviceDisconnected (void)
{
    //USB Device Disconnected
}
