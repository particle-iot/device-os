/**
 ******************************************************************************
 * @file    dfu_hal.c
 * @authors  Satish Nair
 * @version V1.0.0
 * @date    20-Oct-2014
 * @brief   USB DFU Hardware Abstraction Layer
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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
#include "deviceid_hal.h"
#include "bytes2hexbuf.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

uint8_t DeviceState;
uint8_t DeviceStatus[6];

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

uint8_t is_application_valid(uint32_t address)
{
    return (((*(__IO uint32_t*)address) & APP_START_MASK) == 0x20000000);
}

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

const int device_id_len = 12;
/* Read the STM32 Device electronic signature */
unsigned HAL_device_ID(uint8_t* dest, unsigned destLen)
{
    if (dest!=NULL && destLen!=0) {
        memcpy(dest, (char*)ID1, destLen<device_id_len ? destLen : device_id_len);
    }
    return device_id_len;
}

/*******************************************************************************
 * Function Name  : Get_SerialNum.
 * Description    : Create the serial number string descriptor.
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void Get_SerialNum(void)
{
  uint8_t deviceId[16];
  char deviceIdHex[sizeof(deviceId) * 2 + 1] = {0};
  unsigned deviceIdLen = 0;
  deviceIdLen = HAL_device_ID(deviceId, sizeof(deviceId));
  bytes2hexbuf(deviceId, deviceIdLen, deviceIdHex);

  for (unsigned i = 2, pos = 0; i < DFU_SIZ_STRING_SERIAL && pos < 2 * deviceIdLen; i += 2, pos++) {
	DFU_StringSerial[i] = deviceIdHex[pos];
	DFU_StringSerial[i + 1] = '\0';
  }
}


void platform_startup()
{
}
