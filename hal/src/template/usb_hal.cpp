/**
 ******************************************************************************
 * @file    usb_device.c
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    27-Sept-2014
 * @brief   USB Virtual COM Port and HID device HAL
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
#include <stdint.h>

/* Private typedef -----------------------------------------------------------*/

#ifdef USB_CDC_ENABLE
/*******************************************************************************
 * Function Name  : USB_USART_Init
 * Description    : Start USB-USART protocol.
 * Input          : baudRate.
 * Return         : None.
 *******************************************************************************/
void USB_USART_Init(uint32_t baudRate)
{
}

/*******************************************************************************
 * Function Name  : USB_USART_Available_Data.
 * Description    : Return the length of available data received from USB.
 * Input          : None.
 * Return         : Length.
 *******************************************************************************/
uint8_t USB_USART_Available_Data(void)
{
  return 0;
}

/*******************************************************************************
 * Function Name  : USB_USART_Receive_Data.
 * Description    : Return data sent by USB Host.
 * Input          : None
 * Return         : Data.
 *******************************************************************************/
int32_t USB_USART_Receive_Data(uint8_t peek)
{
  return -1;
}

/*******************************************************************************
 * Function Name  : USB_USART_Available_Data_For_Write.
 * Description    : Return the length of available space in TX buffer
 * Input          : None.
 * Return         : Length.
 *******************************************************************************/
int32_t USB_USART_Available_Data_For_Write(void)
{
  return 0;
}

/*******************************************************************************
 * Function Name  : USB_USART_Send_Data.
 * Description    : Send Data from USB_USART to USB Host.
 * Input          : Data.
 * Return         : None.
 *******************************************************************************/
void USB_USART_Send_Data(uint8_t Data)
{

}

/*******************************************************************************
 * Function Name  : USB_USART_Flush_Data.
 * Description    : Flushes TX buffer
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void USB_USART_Flush_Data(void)
{

}

void HAL_USB_Init(void)
{
}

void HAL_USB_Attach()
{
}

void HAL_USB_Detach()
{
}

void HAL_USB_USART_Init(HAL_USB_USART_Serial serial, const HAL_USB_USART_Config* config)
{
}

void HAL_USB_USART_Begin(HAL_USB_USART_Serial serial, uint32_t baud, void *reserved)
{
}

void HAL_USB_USART_End(HAL_USB_USART_Serial serial)
{
}

unsigned int HAL_USB_USART_Baud_Rate(HAL_USB_USART_Serial serial)
{
  return 0;
}

int32_t HAL_USB_USART_Available_Data(HAL_USB_USART_Serial serial)
{
  return -1;
}

int32_t HAL_USB_USART_Available_Data_For_Write(HAL_USB_USART_Serial serial)
{
  return -1;
}

int32_t HAL_USB_USART_Receive_Data(HAL_USB_USART_Serial serial, uint8_t peek)
{
  return -1;
}

int32_t HAL_USB_USART_Send_Data(HAL_USB_USART_Serial serial, uint8_t data)
{
  return -1;
}

void HAL_USB_USART_Flush_Data(HAL_USB_USART_Serial serial)
{
}

bool HAL_USB_USART_Is_Enabled(HAL_USB_USART_Serial serial)
{
  return true;
}

bool HAL_USB_USART_Is_Connected(HAL_USB_USART_Serial serial)
{
  return true;
}

#endif

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
}
#endif



unsigned int USB_USART_Baud_Rate(void)
{
    return 0;
}

void USB_USART_LineCoding_BitRate_Handler(void (*handler)(uint32_t bitRate))
{
}

int32_t USB_USART_Flush_Output(unsigned timeout, void* reserved)
{
    return 0;
}