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
#include <iostream>
#include <stdio.h>
#include <sys/poll.h>
#include <termios.h>
#include <unistd.h>

uint32_t last_baudRate;
/*******************************************************************************
 * Function Name  : USB_USART_Init
 * Description    : Start USB-USART protocol.
 * Input          : baudRate.
 * Return         : None.
 *******************************************************************************/
void USB_USART_Init(uint32_t baudRate)
{
    last_baudRate = baudRate;
    std::cout.setf(std::ios::unitbuf);
}

/*******************************************************************************
 * Function Name  : USB_USART_Available_Data.
 * Description    : Return the length of available data received from USB.
 * Input          : None.
 * Return         : Length.
 *******************************************************************************/
uint8_t USB_USART_Available_Data(void)
{
    struct pollfd stdin_poll = { .fd = STDIN_FILENO
            , .events = POLLIN | POLLRDBAND | POLLRDNORM | POLLPRI };
    int ret = poll(&stdin_poll, 1, 0);
    return ret;
}

int32_t last = -1;

/*******************************************************************************
 * Function Name  : USB_USART_Receive_Data.
 * Description    : Return data sent by USB Host.
 * Input          : None
 * Return         : Data.
 *******************************************************************************/
int32_t USB_USART_Receive_Data(uint8_t peek)
{
    if (last<0 && USB_USART_Available_Data()) {
        uint8_t data = 0;
        if (read(0, &data, 1))
            last = data;
    }
    int32_t result = last;
    if (!peek)
        last = -1;      // consume the data
    return result;
}

/*******************************************************************************
 * Function Name  : USB_USART_Available_Data_For_Write.
 * Description    : Return the length of available space in TX buffer
 * Input          : None.
 * Return         : Length.
 *******************************************************************************/
int32_t USB_USART_Available_Data_For_Write(void)
{
  return 1;
}

/*******************************************************************************
 * Function Name  : USB_USART_Send_Data.
 * Description    : Send Data from USB_USART to USB Host.
 * Input          : Data.
 * Return         : None.
 *******************************************************************************/
void USB_USART_Send_Data(uint8_t Data)
{
    std::cout.write((const char*)&Data, 1);
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
  USB_USART_Init(baud);
}

void HAL_USB_USART_End(HAL_USB_USART_Serial serial)
{
  USB_USART_Init(0);
}

unsigned int HAL_USB_USART_Baud_Rate(HAL_USB_USART_Serial serial)
{
  return USB_USART_Baud_Rate();
}

int32_t HAL_USB_USART_Available_Data(HAL_USB_USART_Serial serial)
{
  return USB_USART_Available_Data();
}

int32_t HAL_USB_USART_Available_Data_For_Write(HAL_USB_USART_Serial serial)
{
  return USB_USART_Available_Data_For_Write();
}

int32_t HAL_USB_USART_Receive_Data(HAL_USB_USART_Serial serial, uint8_t peek)
{
  return USB_USART_Receive_Data(peek);
}

int32_t HAL_USB_USART_Send_Data(HAL_USB_USART_Serial serial, uint8_t data)
{
  USB_USART_Send_Data(data);
  return 1;
}

void HAL_USB_USART_Flush_Data(HAL_USB_USART_Serial serial)
{
  USB_USART_Flush_Data();
}

bool HAL_USB_USART_Is_Enabled(HAL_USB_USART_Serial serial)
{
  return true;
}

bool HAL_USB_USART_Is_Connected(HAL_USB_USART_Serial serial)
{
  return true;
}

#ifdef USB_HID_ENABLE
/*******************************************************************************
 * Function Name : USB_HID_Send_Report.
 * Description   : Send HID Report Info to Host.
 * Input         : pHIDReport and reportSize.
 * Output        : None.
 * Return value  : None.
 *******************************************************************************/
void USB_HID_Send_Report(void *pHIDReport, size_t reportSize)
{
}
#endif



unsigned int USB_USART_Baud_Rate(void)
{
    return last_baudRate;
}

void USB_USART_LineCoding_BitRate_Handler(void (*handler)(uint32_t bitRate))
{
}

int32_t USB_USART_Flush_Output(unsigned timeout, void* reserved)
{
    return 0;
}

#ifdef USB_VENDOR_REQUEST_ENABLE

void HAL_USB_Set_Vendor_Request_Callback(HAL_USB_Vendor_Request_Callback cb, void* p) {
}

void HAL_USB_Set_Vendor_Request_State_Callback(HAL_USB_Vendor_Request_State_Callback cb, void* p) {
}

#endif // defined(USB_VENDOR_REQUEST_ENABLE)
