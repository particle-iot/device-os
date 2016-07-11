/**
  ******************************************************************************
  * @file    usb_endp.cpp
  * @author  Satish Nair
  * @version V1.0.0
  * @date    24-April-2013
  * @brief   Endpoint routines
  ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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
#include "usb_hal.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_mem.h"
#include "usb_istr.h"
#include "usb_pwr.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Interval between sending IN packets in frame number (1 frame = 1ms) */
#define CDC_IN_FRAME_INTERVAL             5

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

extern volatile uint8_t USART_Rx_Buffer[];
extern uint32_t USART_Rx_Buffer_size;
extern volatile uint32_t USART_Rx_ptr_in;
extern volatile uint32_t USART_Rx_ptr_out;
extern volatile uint32_t USART_Rx_length;

extern volatile uint8_t USB_Rx_Buffer[];
extern volatile uint16_t USB_Rx_length;
extern volatile uint16_t USB_Rx_ptr;

extern volatile uint8_t  USB_Tx_State;
extern volatile uint8_t  USB_Rx_State;

extern __IO uint8_t PrevXferComplete;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

#ifdef USB_CDC_ENABLE
/*******************************************************************************
* Function Name  : EP1_IN_Callback.
* Description    : EP1 IN Callback Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void EP1_IN_Callback (void)
{
  uint16_t USB_Tx_ptr;
  uint16_t USB_Tx_length;

  if (USB_Tx_State == 1)
  {
    if (USART_Rx_length == 0)
    {
      USB_Tx_State = 0;
    }
    else
    {
      if (USART_Rx_length > CDC_DATA_SIZE){
        USB_Tx_ptr = USART_Rx_ptr_out;
        USB_Tx_length = CDC_DATA_SIZE;

        USART_Rx_ptr_out += CDC_DATA_SIZE;
        USART_Rx_length -= CDC_DATA_SIZE;
      }
      else
      {
        USB_Tx_ptr = USART_Rx_ptr_out;
        USB_Tx_length = USART_Rx_length;

        USART_Rx_ptr_out += USART_Rx_length;
        USART_Rx_length = 0;
      }
      UserToPMABufferCopy(&USART_Rx_Buffer[USB_Tx_ptr], ENDP1_TXADDR, USB_Tx_length);
      SetEPTxCount(ENDP1, USB_Tx_length);
      SetEPTxValid(ENDP1);
    }
  }
}

/*******************************************************************************
* Function Name  : EP3_OUT_Callback
* Description    : EP3 OUT Callback Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void EP3_OUT_Callback(void)
{
  /* Get the number of received data on the selected Endpoint */
  USB_Rx_length = GetEPRxCount(ENDP3);

  /* Use the memory interface function to write to the selected endpoint */
  PMAToUserBufferCopy(USB_Rx_Buffer, ENDP3_RXADDR, USB_Rx_length);

  /* USB data should be immediately processed, this allow next USB traffic being
  NAKed till the end of the processing */
  USB_Rx_State = 1;

  USB_Rx_ptr = 0;


}

/*******************************************************************************
 * Function Name  : Handle_USBAsynchXfer.
 * Description    : send data to USB.
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void Handle_USBAsynchXfer (void)
{
	uint16_t USB_Tx_ptr;
	uint16_t USB_Tx_length;

	if(USB_Tx_State != 1)
	{
		if (USART_Rx_ptr_out == USART_Rx_Buffer_size)
		{
			USART_Rx_ptr_out = 0;
		}

		if(USART_Rx_ptr_out == USART_Rx_ptr_in)
		{
			USB_Tx_State = 0;
			return;
		}

		if(USART_Rx_ptr_out > USART_Rx_ptr_in) /* rollback */
		{
			USART_Rx_length = USART_Rx_Buffer_size - USART_Rx_ptr_out;
		}
		else
		{
			USART_Rx_length = USART_Rx_ptr_in - USART_Rx_ptr_out;
		}

		if (USART_Rx_length > CDC_DATA_SIZE)
		{
			USB_Tx_ptr = USART_Rx_ptr_out;
			USB_Tx_length = CDC_DATA_SIZE;

			USART_Rx_ptr_out += CDC_DATA_SIZE;
			USART_Rx_length -= CDC_DATA_SIZE;
		}
		else
		{
			USB_Tx_ptr = USART_Rx_ptr_out;
			USB_Tx_length = USART_Rx_length;

			USART_Rx_ptr_out += USART_Rx_length;
			USART_Rx_length = 0;
		}
		UserToPMABufferCopy(&USART_Rx_Buffer[USB_Tx_ptr], ENDP1_TXADDR, USB_Tx_length);
		SetEPTxCount(ENDP1, USB_Tx_length);
		SetEPTxValid(ENDP1);
		USB_Tx_State = 1;
	}
}

/*******************************************************************************
* Function Name  : SOF_Callback / INTR_SOFINTR_Callback
* Description    : SOF Callback Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void SOF_Callback(void)
{
  static uint32_t FrameCount = 0;

  if(bDeviceState == CONFIGURED)
  {
    if (FrameCount++ == CDC_IN_FRAME_INTERVAL)
    {
      /* Reset the frame counter */
      FrameCount = 0;

      /* Check the data to be sent through IN pipe */
      Handle_USBAsynchXfer();
    }
  }
}
#endif

#ifdef USB_HID_ENABLE
/*******************************************************************************
* Function Name  : EP1_IN_Callback.
* Description    : EP1 IN Callback Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void EP1_IN_Callback(void)
{
  /* Set the transfer complete token to inform upper layer that the current
  transfer has been complete */
  PrevXferComplete = 1;
}
#endif
