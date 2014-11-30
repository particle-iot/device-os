/**
 ******************************************************************************
 * @file    usbd_usr.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    21-Oct-2014
 * @brief   This file includes the user application layer
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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

uint8_t State = 0;

/**
 * @brief  USBD_USR_Init
 * @param  None
 * @retval None
 */
void USBD_USR_Init(void)
{
    //Additional application hooks - update this section as per requirement
    //User initialization code if any (not required now for core-v2)
}

/**
 * @brief  USBD_USR_DeviceReset
 * @param  speed : device speed
 * @retval None
 */
void USBD_USR_DeviceReset(uint8_t speed )
{
    //Additional application hooks - update this section as per requirement
    switch (speed)
    {
        case USB_OTG_SPEED_HIGH:
            //USB Device Library v1.1.0 [HS]
            break;

        case USB_OTG_SPEED_FULL:
            //USB Device Library v1.1.0 [FS]
            break;
        default:
            //USB Device Library v1.1.0 [??]
            break;
    }
}


/**
 * @brief  USBD_USR_DeviceConfigured
 * @param  None
 * @retval Staus
 */
void USBD_USR_DeviceConfigured (void)
{
    //Additional application hooks - update this section as per requirement
    if (State == 0)
    {
        //DFU Interface Started
    }
    else if (State == 1)
    {
        //DFU Interface Configured
    }

    State++;
}
/**
 * @brief  USBD_USR_DeviceSuspended
 * @param  None
 * @retval None
 */
void USBD_USR_DeviceSuspended(void)
{
    //Additional application hooks - update this section as per requirement
    //> USB Device in Suspend Mode
    /* Users can do their application actions here for the USB-Reset */
}


/**
 * @brief  USBD_USR_DeviceResumed
 * @param  None
 * @retval None
 */
void USBD_USR_DeviceResumed(void)
{
    //Additional application hooks - update this section as per requirement
    //> USB Device in Idle Mode
    /* Users can do their application actions here for the USB-Reset */
}

/**
 * @brief  USBD_USR_DeviceConnected
 * @param  None
 * @retval Staus
 */
void USBD_USR_DeviceConnected (void)
{
    //Additional application hooks - update this section as per requirement
    //> USB Device Connected
}


/**
 * @brief  USBD_USR_DeviceDisonnected
 * @param  None
 * @retval Staus
 */
void USBD_USR_DeviceDisconnected (void)
{
    //Additional application hooks - update this section as per requirement
    //> USB Device Disconnected
}
