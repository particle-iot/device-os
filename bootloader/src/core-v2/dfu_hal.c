/**
 ******************************************************************************
 * @file    dfu_hal.c
 * @authors  Satish Nair
 * @version V1.0.0
 * @date    20-Oct-2014
 * @brief   USB DFU Hardware Abstraction Layer
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "usbd_dfu_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include "usb_conf.h"
#include "core_hal.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
USB_OTG_CORE_HANDLE USB_OTG_dev;

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
 * Function Name  : HAL_DFU_USB_Init.
 * Description    : Initialize USB for DFU class.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_DFU_USB_Init(void)
{
    USBD_Init(&USB_OTG_dev,
#ifdef USE_USB_OTG_FS
            USB_OTG_FS_CORE_ID,
#elif defined USE_USB_OTG_HS
            USB_OTG_HS_CORE_ID,
#endif
            &USR_desc,
            &DFU_cb,
            &USR_cb);
}

int32_t HAL_Core_Backup_Register(uint32_t BKP_DR)
{
    switch(BKP_DR)
    {
        case BKP_DR_01: return RTC_BKP_DR1;  break;
        case BKP_DR_02: return RTC_BKP_DR2;  break;
        case BKP_DR_03: return RTC_BKP_DR3;  break;
        case BKP_DR_04: return RTC_BKP_DR4;  break;
        case BKP_DR_05: return RTC_BKP_DR5;  break;
        case BKP_DR_06: return RTC_BKP_DR6;  break;
        case BKP_DR_07: return RTC_BKP_DR7;  break;
        case BKP_DR_08: return RTC_BKP_DR8;  break;
        case BKP_DR_09: return RTC_BKP_DR9;  break;
        case BKP_DR_10: return RTC_BKP_DR10; break;
    }
    return -1;
}

void HAL_Core_Write_Backup_Register(uint32_t BKP_DR, uint32_t Data)
{
    uint32_t BKP_DR_Address = HAL_Core_Backup_Register(BKP_DR);
    if(BKP_DR_Address != -1)
    {
        RTC_WriteBackupRegister(BKP_DR_Address, Data);
    }
}

uint32_t HAL_Core_Read_Backup_Register(uint32_t BKP_DR)
{
    uint32_t BKP_DR_Address = HAL_Core_Backup_Register(BKP_DR);
    if(BKP_DR_Address != -1)
    {
        return RTC_ReadBackupRegister(BKP_DR_Address);
    }
    return 0xFFFFFFFF;
}
