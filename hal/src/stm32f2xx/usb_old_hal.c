/**
  Copyright (c) 2013-2016 Particle Industries, Inc.  All rights reserved.

  Copyright 2012 STMicroelectronics
  http://www.st.com/software_license_agreement_liberty_v2

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
#include "platform_headers.h"
#include "usbd_usr.h"
#include "usb_conf.h"
#include "usbd_desc.h"
#include "delay_hal.h"
#include "interrupts_hal.h"
#include "usbd_composite.h"
#include "usbd_mcdc.h"
#include "usbd_mhid.h"
#include "debug.h"
#include "usbd_desc_device.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/


#ifdef USB_CDC_ENABLE

void USB_USART_LineCoding_BitRate_Handler(void (*handler)(uint32_t bitRate))
{
    // Enable Serial by default
	HAL_USB_USART_LineCoding_BitRate_Handler(handler, NULL);
}

/************************************************************************************************************/
/* Compatibility */
/************************************************************************************************************/
void USB_USART_Init(uint32_t baudRate)
{
	static uint8_t inited = 0;

	if (!inited) {
		inited = 1;
        // For compatibility we allocate buffers, as application calling USB_USART_Init
        // assumes that the driver has its own buffers
        HAL_USB_USART_Config conf;
        memset(&conf, 0, sizeof(conf));
        HAL_USB_USART_Init(HAL_USB_USART_SERIAL, &conf);
    }
    if (HAL_USB_USART_Baud_Rate(HAL_USB_USART_SERIAL) != baudRate)
    {
        if (!baudRate && HAL_USB_USART_Baud_Rate(HAL_USB_USART_SERIAL) > 0)
        {
            HAL_USB_USART_End(HAL_USB_USART_SERIAL);
        }
        else if (!HAL_USB_USART_Baud_Rate(HAL_USB_USART_SERIAL))
        {
           HAL_USB_USART_Begin(HAL_USB_USART_SERIAL, baudRate, NULL);
        }
    }
}

unsigned USB_USART_Baud_Rate(void)
{
    return HAL_USB_USART_Baud_Rate(HAL_USB_USART_SERIAL);
}

/*******************************************************************************
 * Function Name  : USB_USART_Available_Data.
 * Description    : Return the length of available data received from USB.
 * Input          : None.
 * Return         : Length.
 *******************************************************************************/
uint8_t USB_USART_Available_Data(void)
{
    int32_t available = HAL_USB_USART_Available_Data(HAL_USB_USART_SERIAL);
    if (available > 255)
        return 255;
    else if (available < 0)
        return 0;
    return available;
}

/*******************************************************************************
 * Function Name  : USB_USART_Receive_Data.
 * Description    : Return data sent by USB Host.
 * Input          : None
 * Return         : Data.
 *******************************************************************************/
int32_t USB_USART_Receive_Data(uint8_t peek)
{
    return HAL_USB_USART_Receive_Data(HAL_USB_USART_SERIAL, peek);
}

/*******************************************************************************
 * Function Name  : USB_USART_Available_Data_For_Write.
 * Description    : Return the length of available space in TX buffer
 * Input          : None.
 * Return         : Length.
 *******************************************************************************/
int32_t USB_USART_Available_Data_For_Write(void)
{
    return HAL_USB_USART_Available_Data_For_Write(HAL_USB_USART_SERIAL);
}

/*******************************************************************************
 * Function Name  : USB_USART_Send_Data.
 * Description    : Send Data from USB_USART to USB Host.
 * Input          : Data.
 * Return         : None.
 *******************************************************************************/
void USB_USART_Send_Data(uint8_t Data)
{
    HAL_USB_USART_Send_Data(HAL_USB_USART_SERIAL, Data);
}

/*******************************************************************************
 * Function Name  : USB_USART_Flush_Data.
 * Description    : Flushes TX buffer
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void USB_USART_Flush_Data(void)
{
    HAL_USB_USART_Flush_Data(HAL_USB_USART_SERIAL);
}
#endif /* USB_CDC_ENABLE */

#ifdef USB_HID_ENABLE
/*******************************************************************************
 * Function Name : USB_HID_Send_Report.
 * Description   : Send HID Report Info to Host.
 * Input         : pHIDReport and reportSize.
 * Output        : None.
 * Return value  : None.
 *******************************************************************************/
void USB_HID_Send_Report(void *pHIDReport, uint16_t reportSize)
{
    HAL_USB_HID_Send_Report(0, pHIDReport, reportSize, NULL);
}
#endif


