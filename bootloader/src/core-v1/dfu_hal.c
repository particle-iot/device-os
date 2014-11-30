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
#include "usb_hal.h"
#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_prop.h"
#include "usb_pwr.h"
#include "dfu_mal.h"
#include "core_hal.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

uint8_t DeviceState;
uint8_t DeviceStatus[6];

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len);

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
    // Enter DFU mode
    DeviceState = STATE_dfuERROR;
    DeviceStatus[0] = STATUS_ERRFIRMWARE;
    DeviceStatus[4] = DeviceState;

    // Unlock the internal flash
    FLASH_Unlock();

    // USB Disconnect configuration
    USB_Disconnect_Config();

    // Disable the USB connection till initialization phase end
    USB_Cable_Config(DISABLE);

    // Init the media interface
    MAL_Init();

    // Enable the USB connection
    USB_Cable_Config(ENABLE);

    // USB Clock configuration
    Set_USBClock();

    // USB System initialization
    USB_Init();
}

int32_t HAL_Core_Backup_Register(uint32_t BKP_DR)
{
    switch(BKP_DR)
    {
        case BKP_DR_01: return BKP_DR1;  break;
        case BKP_DR_02: return BKP_DR2;  break;
        case BKP_DR_03: return BKP_DR3;  break;
        case BKP_DR_04: return BKP_DR4;  break;
        case BKP_DR_05: return BKP_DR5;  break;
        case BKP_DR_06: return BKP_DR6;  break;
        case BKP_DR_07: return BKP_DR7;  break;
        case BKP_DR_08: return BKP_DR8;  break;
        case BKP_DR_09: return BKP_DR9;  break;
        case BKP_DR_10: return BKP_DR10; break;
    }
    return -1;
}

void HAL_Core_Write_Backup_Register(uint32_t BKP_DR, uint32_t Data)
{
    uint32_t BKP_DR_Address = HAL_Core_Backup_Register(BKP_DR);
    if(BKP_DR_Address != -1)
    {
        BKP_WriteBackupRegister(BKP_DR_Address, Data);
    }
}

uint32_t HAL_Core_Read_Backup_Register(uint32_t BKP_DR)
{
    uint32_t BKP_DR_Address = HAL_Core_Backup_Register(BKP_DR);
    if(BKP_DR_Address != -1)
    {
        return BKP_ReadBackupRegister(BKP_DR_Address);
    }
    return 0xFFFFFFFF;
}

/*******************************************************************************
 * Function Name  : Get_SerialNum.
 * Description    : Create the serial number string descriptor.
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void Get_SerialNum(void)
{
    uint32_t Device_Serial0, Device_Serial1, Device_Serial2;

    Device_Serial0 = *(uint32_t*)ID1;
    Device_Serial1 = *(uint32_t*)ID2;
    Device_Serial2 = *(uint32_t*)ID3;

    Device_Serial0 += Device_Serial2;

    if (Device_Serial0 != 0)
    {
        IntToUnicode (Device_Serial0, &DFU_StringSerial[2] , 8);
        IntToUnicode (Device_Serial1, &DFU_StringSerial[18], 4);
    }
}

/*******************************************************************************
 * Function Name  : HexToChar.
 * Description    : Convert Hex 32Bits value into char.
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len)
{
    uint8_t idx = 0;

    for( idx = 0 ; idx < len ; idx ++)
    {
        if( ((value >> 28)) < 0xA )
        {
            pbuf[ 2* idx] = (value >> 28) + '0';
        }
        else
        {
            pbuf[2* idx] = (value >> 28) + 'A' - 10;
        }

        value = value << 4;

        pbuf[ 2* idx + 1] = 0;
    }
}
