/**
 ******************************************************************************
 * @file    usbd_cdc_if.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    26-Feb-2015
 * @brief   Generic media access Layer.
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
  Copyright (c) 2012 STMicroelectronics

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

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
#pragma     data_alignment = 4
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */

/* Includes ------------------------------------------------------------------*/
#include "usb_conf.h"
#include "usbd_conf.h"
#include "usbd_cdc_core.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
volatile LINE_CODING linecoding =
{
    0x00,   /* baud rate*/
    0x00,   /* stop bits*/
    0x00,   /* parity*/
    0x00    /* nb. of bits */
};

static linecoding_bitrate_handler APP_LineCodingBitRateHandler = NULL;

/* These are external variables imported from CDC core to be used for IN
   transfer management. */
extern volatile uint8_t  APP_Rx_Buffer []; /* Write CDC received data in this buffer.
                                     These data will be sent over USB IN endpoint
                                     in the CDC core functions. */
extern volatile uint32_t APP_Rx_ptr_in;    /* Increment this pointer or roll it back to
                                     start address when writing received data
                                     in the buffer APP_Rx_Buffer. */

/* Private function prototypes -----------------------------------------------*/
static uint16_t APP_Init     (void);
static uint16_t APP_DeInit   (void);
static uint16_t APP_Ctrl     (uint32_t Cmd, uint8_t* Buf, uint32_t Len);
static uint16_t APP_DataTx   (uint8_t* Buf, uint32_t Len);
static uint16_t APP_DataRx   (uint8_t* Buf, uint32_t Len);

CDC_IF_Prop_TypeDef APP_fops =
{
        APP_Init,
        APP_DeInit,
        APP_Ctrl,
        APP_DataTx,
        APP_DataRx
};

/* Private functions ---------------------------------------------------------*/

void SetLineCodingBitRateHandler(linecoding_bitrate_handler handler)
{
    APP_LineCodingBitRateHandler = handler;
}

/**
 * @brief  APP_Init
 *         Initializes the CDC APP low layer
 * @param  None
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static uint16_t APP_Init(void)
{
    //Do Nothing
    return USBD_OK;
}

/**
 * @brief  APP_DeInit
 *         DeInitializes the CDC APP low layer
 * @param  None
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static uint16_t APP_DeInit(void)
{
    //Do Nothing
    return USBD_OK;
}

/**
 * @brief  APP_Ctrl
 *         Manage the CDC class requests
 * @param  Cmd: Command code
 * @param  Buf: Buffer containing command data (request parameters)
 * @param  Len: Number of data to be sent (in bytes)
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static uint16_t APP_Ctrl (uint32_t Cmd, uint8_t* Buf, uint32_t Len)
{
    switch (Cmd)
    {
    case SEND_ENCAPSULATED_COMMAND:
        /* Not needed for this driver */
        break;

    case GET_ENCAPSULATED_RESPONSE:
        /* Not needed for this driver */
        break;

    case SET_COMM_FEATURE:
        /* Not needed for this driver */
        break;

    case GET_COMM_FEATURE:
        /* Not needed for this driver */
        break;

    case CLEAR_COMM_FEATURE:
        /* Not needed for this driver */
        break;

    case SET_LINE_CODING:
        linecoding.bitrate = (uint32_t)(Buf[0] | (Buf[1] << 8) | (Buf[2] << 16) | (Buf[3] << 24));
        linecoding.format = Buf[4];
        linecoding.paritytype = Buf[5];
        linecoding.datatype = Buf[6];

        //Callback handler when the host sets a specific linecoding
        if (NULL != APP_LineCodingBitRateHandler)
        {
            APP_LineCodingBitRateHandler(linecoding.bitrate);
        }
        break;

    case GET_LINE_CODING:
        Buf[0] = (uint8_t)(linecoding.bitrate);
        Buf[1] = (uint8_t)(linecoding.bitrate >> 8);
        Buf[2] = (uint8_t)(linecoding.bitrate >> 16);
        Buf[3] = (uint8_t)(linecoding.bitrate >> 24);
        Buf[4] = linecoding.format;
        Buf[5] = linecoding.paritytype;
        Buf[6] = linecoding.datatype;
        break;

    case SET_CONTROL_LINE_STATE:
        /* Not needed for this driver */
        break;

    case SEND_BREAK:
        /* Not needed for this driver */
        break;

    default:
        break;
    }

    return USBD_OK;
}

/**
 * @brief  APP_DataTx
 *         CDC received data to be send over USB IN endpoint are managed in
 *         this function.
 * @param  Buf: Buffer of data to be sent
 * @param  Len: Number of data to be sent (in bytes)
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static uint16_t APP_DataTx (uint8_t* Buf, uint32_t Len)
{
    //Do Nothing
    return USBD_OK;
}

/**
 * @brief  APP_DataRx
 *         Data received over USB OUT endpoint are sent over CDC interface
 *         through this function.
 *
 *         @note
 *         This function will block any OUT packet reception on USB endpoint
 *         untill exiting this function. If you exit this function before transfer
 *         is complete on CDC interface (ie. using DMA controller) it will result
 *         in receiving more data while previous ones are still not sent.
 *
 * @param  Buf: Buffer of data to be received
 * @param  Len: Number of data received (in bytes)
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static uint16_t APP_DataRx (uint8_t* Buf, uint32_t Len)
{
    //Do Nothing
    return USBD_OK;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
