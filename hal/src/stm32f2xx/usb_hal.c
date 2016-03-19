/**
 ******************************************************************************
 * @file    usb_hal.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    05-Nov-2014
 * @brief   USB Virtual COM Port and HID device HAL
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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
#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usb_conf.h"
#include "usbd_desc.h"
#include "delay_hal.h"
#include "interrupts_hal.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
USB_OTG_CORE_HANDLE USB_OTG_dev;

extern uint32_t USBD_OTG_ISR_Handler(USB_OTG_CORE_HANDLE *pdev);
#ifdef USB_OTG_HS_DEDICATED_EP1_ENABLED
extern uint32_t USBD_OTG_EP1IN_ISR_Handler(USB_OTG_CORE_HANDLE *pdev);
extern uint32_t USBD_OTG_EP1OUT_ISR_Handler(USB_OTG_CORE_HANDLE *pdev);
#endif

/* Extern variables ----------------------------------------------------------*/
#ifdef USB_CDC_ENABLE
extern volatile LINE_CODING linecoding;
extern volatile uint8_t USB_DEVICE_CONFIGURED;
extern volatile uint8_t USB_Rx_Buffer[];
extern volatile uint32_t USB_Rx_Buffer_head;
extern volatile uint32_t USB_Rx_Buffer_tail;
extern volatile uint32_t USB_Rx_Buffer_length;
extern volatile uint8_t USB_Tx_Buffer[];
extern volatile uint32_t USB_Tx_Buffer_head;
extern volatile uint32_t USB_Tx_Buffer_tail;
extern volatile uint8_t  USB_Tx_State;
extern volatile uint8_t  USB_Rx_State;
extern volatile uint8_t  USB_Serial_Open;
#endif

#if defined (USB_CDC_ENABLE) || defined (USB_HID_ENABLE)
/*******************************************************************************
 * Function Name  : SPARK_USB_Setup
 * Description    : Spark USB Setup.
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void SPARK_USB_Setup(void)
{
    USBD_Init(&USB_OTG_dev,
#ifdef USE_USB_OTG_FS
            USB_OTG_FS_CORE_ID,
#elif defined USE_USB_OTG_HS
            USB_OTG_HS_CORE_ID,
#endif
            &USR_desc,
            &USBD_CDC_cb,
            NULL);
}

/*******************************************************************************
 * Function Name  : Get_SerialNum.
 * Description    : Create the serial number string descriptor.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void Get_SerialNum(void)
{
    //Not required. Retained for compatibility
}
#endif

#ifdef USB_CDC_ENABLE
/*******************************************************************************
 * Function Name  : USB_USART_Init
 * Description    : Start USB-USART protocol.
 * Input          : baudRate (0 : disconnect usb else init usb).
 * Return         : None.
 *******************************************************************************/
void USB_USART_Init(uint32_t baudRate)
{
    if (linecoding.bitrate != baudRate)
    {
        if (!baudRate && linecoding.bitrate > 0)
        {
            // Deconfigure CDC class endpoints
            USBD_ClrCfg(&USB_OTG_dev, 0);

            // Class callbacks and descriptor should probably be cleared
            // to use another USB class, but this causes a hardfault if no descriptor is set
            // and detach/attach is performed.
            // Leave them be for now.

            // USB_OTG_dev.dev.class_cb = NULL;
            // USB_OTG_dev.dev.usr_cb = NULL;
            // USB_OTG_dev.dev.usr_device = NULL;

            // Perform detach
            USB_Cable_Config(DISABLE);

            // Soft reattach
            // USB_OTG_dev.regs.DREGS->DCTL |= 0x02;
        }
        else if (!linecoding.bitrate)
        {
            //Initialize USB device
            SPARK_USB_Setup();

            // Perform a hard Detach-Attach operation on USB bus
            USB_Cable_Config(DISABLE);
            USB_Cable_Config(ENABLE);

            // Soft reattach
            // USB_OTG_dev.regs.DREGS->DCTL |= 0x02;
        }
        //linecoding.bitrate will be overwritten by USB Host
        linecoding.bitrate = baudRate;
    }
}

unsigned USB_USART_Baud_Rate(void)
{
    return linecoding.bitrate;
}

void USB_USART_LineCoding_BitRate_Handler(void (*handler)(uint32_t bitRate))
{
    //Init USB Serial first before calling the linecoding handler
    USB_USART_Init(9600);

    // When using Arduino avrdude to upload user sketch to Duo, 
    // this delay maybe make it miss the start signal from Arduino avrdude.
#if PLATFORM_ID != 88
    HAL_Delay_Milliseconds(1000);
#endif

    //Set the system defined custom handler
    SetLineCodingBitRateHandler(handler);
}

static inline bool USB_USART_Connected() {
    return linecoding.bitrate > 0 && USB_OTG_dev.dev.device_status == USB_OTG_CONFIGURED && USB_Serial_Open;
}

/*******************************************************************************
 * Function Name  : USB_USART_Available_Data.
 * Description    : Return the length of available data received from USB.
 * Input          : None.
 * Return         : Length.
 *******************************************************************************/
uint8_t USB_USART_Available_Data(void)
{
    int32_t available = 0;
    int state = HAL_disable_irq();
    if (USB_Rx_Buffer_head >= USB_Rx_Buffer_tail)
        available = USB_Rx_Buffer_head - USB_Rx_Buffer_tail;
    else
        available = USB_Rx_Buffer_length + USB_Rx_Buffer_head - USB_Rx_Buffer_tail;
    HAL_enable_irq(state);
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
    if (USB_USART_Available_Data() > 0)
    {
        int state = HAL_disable_irq();
        uint8_t data = USB_Rx_Buffer[USB_Rx_Buffer_tail];
        if (!peek) {
            USB_Rx_Buffer_tail++;
            if (USB_Rx_Buffer_tail == USB_Rx_Buffer_length)
                USB_Rx_Buffer_tail = 0;
        }
        HAL_enable_irq(state);
        return data;
    }

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
    if (USB_USART_Connected())
    {
        uint32_t tail = USB_Tx_Buffer_tail;
        int32_t available = USB_TX_BUFFER_SIZE - (USB_Tx_Buffer_head >= tail ?
            USB_Tx_Buffer_head - tail : USB_TX_BUFFER_SIZE + USB_Tx_Buffer_head - tail) - 1;
        return available;
    }

    return -1;
}

/*******************************************************************************
 * Function Name  : USB_USART_Send_Data.
 * Description    : Send Data from USB_USART to USB Host.
 * Input          : Data.
 * Return         : None.
 *******************************************************************************/
void USB_USART_Send_Data(uint8_t Data)
{
    int32_t available = 0;
    do {
        available = USB_USART_Available_Data_For_Write();
    }
    while (available < 1 && available != -1);
    // Confirm once again that the Host is connected
    if (USB_USART_Connected())
    {
        uint32_t head = USB_Tx_Buffer_head;

        USB_Tx_Buffer[head] = Data;

        USB_Tx_Buffer_head = ++head % USB_TX_BUFFER_SIZE;
    }
}

/*******************************************************************************
 * Function Name  : USB_USART_Flush_Data.
 * Description    : Flushes TX buffer
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void USB_USART_Flush_Data(void)
{
    while(USB_USART_Connected() && USB_USART_Available_Data_For_Write() != (USB_TX_BUFFER_SIZE - 1));
    // We should also wait for USB_Tx_State to become 0, as hardware might still be busy transmitting data
    while(USB_Tx_State == 1);
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
void USB_HID_Send_Report(void *pHIDReport, size_t reportSize)
{
    //To Do
}
#endif

#ifdef USE_USB_OTG_FS
/**
 * @brief  This function handles OTG_FS_WKUP Handler.
 * @param  None
 * @retval None
 */
void OTG_FS_WKUP_irq(void)
{
    if(USB_OTG_dev.cfg.low_power)
    {
        *(uint32_t *)(0xE000ED10) &= 0xFFFFFFF9 ;
        SystemInit();
        USB_OTG_UngateClock(&USB_OTG_dev);
    }
    EXTI_ClearITPendingBit(EXTI_Line18);
}
#elif defined USE_USB_OTG_HS
/**
 * @brief  This function handles OTG_HS_WKUP Handler.
 * @param  None
 * @retval None
 */
void OTG_HS_WKUP_irq(void)
{
    if(USB_OTG_dev.cfg.low_power)
    {
        *(uint32_t *)(0xE000ED10) &= 0xFFFFFFF9 ;
        SystemInit();
        USB_OTG_UngateClock(&USB_OTG_dev);
    }
    EXTI_ClearITPendingBit(EXTI_Line20);
}
#endif

#ifdef USE_USB_OTG_FS
/**
 * @brief  This function handles OTG_FS Handler.
 * @param  None
 * @retval None
 */
void OTG_FS_irq(void)
{
    USBD_OTG_ISR_Handler(&USB_OTG_dev);
}
#elif defined USE_USB_OTG_HS
/**
 * @brief  This function handles OTG_HS Handler.
 * @param  None
 * @retval None
 */
void OTG_HS_irq(void)
{
    USBD_OTG_ISR_Handler(&USB_OTG_dev);
}
#endif

#ifdef USB_OTG_HS_DEDICATED_EP1_ENABLED
/**
 * @brief  This function handles OTG_HS_EP1_IN_IRQ Handler.
 * @param  None
 * @retval None
 */
void OTG_HS_EP1_IN_irq(void)
{
    USBD_OTG_EP1IN_ISR_Handler (&USB_OTG_dev);
}

/**
 * @brief  This function handles OTG_HS_EP1_OUT_IRQ Handler.
 * @param  None
 * @retval None
 */
void OTG_HS_EP1_OUT_irq(void)
{
    USBD_OTG_EP1OUT_ISR_Handler (&USB_OTG_dev);
}
#endif
