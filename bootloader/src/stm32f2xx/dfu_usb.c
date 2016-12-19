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
static uint8_t HAL_DFU_USB_Handle_Vendor_Request(USB_SETUP_REQ* req, uint8_t dataStage);
/* Private functions ---------------------------------------------------------*/

uint8_t is_application_valid(uint32_t address)
{
#ifdef FLASH_UPDATE_MODULES
    return FLASH_isUserModuleInfoValid(FLASH_INTERNAL, address, address);
#else
    return (((*(__IO uint32_t*)address) & APP_START_MASK) == 0x20000000);
#endif
}

static void dummy(void* reserved) {}

USBD_Usr_cb_TypeDef DFU_USR_cb =
{
        (void(*)(void))dummy, // USBD_USR_Init,
        (void(*)(uint8_t))dummy, // USBD_USR_DeviceReset,
        (void(*)(void))dummy, // USBD_USR_DeviceConfigured,
        (void(*)(void))dummy, // USBD_USR_DeviceSuspended,
        (void(*)(void))dummy, // USBD_USR_DeviceResumed,
        (void(*)(void))dummy, // USBD_USR_DeviceConnected,
        (void(*)(void))dummy, // USBD_USR_DeviceDisconnected,
        HAL_DFU_USB_Handle_Vendor_Request
};

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
              &USR_desc, &DFU_cb,
              &DFU_USR_cb); // Passing NULL here to reduce bootloader flash requirements
}

uint8_t HAL_DFU_USB_Handle_Vendor_Request(USB_SETUP_REQ* req, uint8_t dataStage) {
    // Forward to DFU class driver
    if (req != NULL && req->bRequest == 0xee && req->wIndex == 0x0004 && req->wValue == 0x0000) {
        return DFU_cb.Setup(&USB_OTG_dev, req);
    }

    return USBD_FAIL;
}
