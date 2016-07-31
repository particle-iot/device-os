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
#include "usb_settings.h"
#include "usbd_mcdc.h"
#include "usbd_usr.h"
#include "usb_conf.h"
#include "usbd_desc.h"
#include "delay_hal.h"
#include "interrupts_hal.h"
#include "ringbuf_helper.h"
#include <string.h>

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
static void (*LineCoding_BitRate_Handler)(uint32_t bitRate) = NULL;
USBD_MCDC_Instance_Data USBD_MCDC = {{0}};
__ALIGN_BEGIN static uint8_t USBD_MCDC_Rx_Buffer[USB_RX_BUFFER_SIZE];
__ALIGN_BEGIN static uint8_t USBD_MCDC_Tx_Buffer[USB_TX_BUFFER_SIZE];
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
            &USBD_MCDC_cb,
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

uint16_t USB_USART_Request_Handler(/*USBD_Composite_Class_Data* cls, */uint32_t cmd, uint8_t* buf, uint32_t len) {
    if (cmd == SET_LINE_CODING && LineCoding_BitRate_Handler) {
        // USBD_MCDC_Instance_Data* priv = (USBD_MCDC_Instance_Data*)cls->priv;
        USBD_MCDC_Instance_Data* priv = &USBD_MCDC;
        if (priv)
            LineCoding_BitRate_Handler(priv->linecoding.bitrate);
    }

    return 0;
}

/*******************************************************************************
 * Function Name  : USB_USART_Init
 * Description    : Start USB-USART protocol.
 * Input          : baudRate (0 : disconnect usb else init usb).
 * Return         : None.
 *******************************************************************************/
void USB_USART_Init(uint32_t baudRate)
{
    if (USBD_MCDC.linecoding.bitrate != baudRate)
    {
        if (!baudRate && USBD_MCDC.linecoding.bitrate > 0)
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
        else if (!USBD_MCDC.linecoding.bitrate)
        {
            memset((void*)&USBD_MCDC, 0, sizeof(USBD_MCDC));
            USBD_MCDC.ep_in_data = CDC_IN_EP;
            USBD_MCDC.ep_in_int = CDC_CMD_EP;
            USBD_MCDC.ep_out_data = CDC_OUT_EP;
            USBD_MCDC.rx_buffer = USBD_MCDC_Rx_Buffer;
            USBD_MCDC.tx_buffer = USBD_MCDC_Tx_Buffer;
            USBD_MCDC.rx_buffer_size = USB_RX_BUFFER_SIZE;
            USBD_MCDC.tx_buffer_size = USB_TX_BUFFER_SIZE;
            USBD_MCDC.name = "Serial";
            USBD_MCDC.req_handler = USB_USART_Request_Handler;
            //Initialize USB device
            SPARK_USB_Setup();

            // Perform a hard Detach-Attach operation on USB bus
            USB_Cable_Config(DISABLE);
            USB_Cable_Config(ENABLE);

            // Soft reattach
            // USB_OTG_dev.regs.DREGS->DCTL |= 0x02;
        }
        //linecoding.bitrate will be overwritten by USB Host
        // linecoding.bitrate = baudRate;
    }
}

unsigned USB_USART_Baud_Rate(void)
{
    return USBD_MCDC.linecoding.bitrate;
}

void USB_USART_LineCoding_BitRate_Handler(void (*handler)(uint32_t bitRate))
{
    //Init USB Serial first before calling the linecoding handler
    USB_USART_Init(9600);

    LineCoding_BitRate_Handler = handler;
}

static inline bool USB_USART_Connected() {
    return USBD_MCDC.linecoding.bitrate > 0 && USB_OTG_dev.dev.device_status == USB_OTG_CONFIGURED && USBD_MCDC.serial_open;
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
    available = ring_data_avail(USBD_MCDC.rx_buffer_length,
                                USBD_MCDC.rx_buffer_head,
                                USBD_MCDC.rx_buffer_tail);
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
        uint8_t data = USBD_MCDC.rx_buffer[USBD_MCDC.rx_buffer_tail];
        if (!peek) {
            USBD_MCDC.rx_buffer_tail = ring_wrap(USBD_MCDC.rx_buffer_length, USBD_MCDC.rx_buffer_tail + 1);
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
        int state = HAL_disable_irq();
        int32_t available = ring_space_avail(USBD_MCDC.tx_buffer_size,
                                             USBD_MCDC.tx_buffer_head,
                                             USBD_MCDC.tx_buffer_tail);
        HAL_enable_irq(state);
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
void USB_USART_Send_Data(uint8_t data)
{
    int32_t ret = -1;
    int32_t available = 0;
    do {
        available = USB_USART_Available_Data_For_Write();
    }
    while (available < 1 && available != -1);
    // Confirm once again that the Host is connected
    int32_t state = HAL_disable_irq();
    if (USB_USART_Connected() && available > 0)
    {
        USBD_MCDC.tx_buffer[USBD_MCDC.tx_buffer_head] = data;
        USBD_MCDC.tx_buffer_head = ring_wrap(USBD_MCDC.tx_buffer_size, USBD_MCDC.tx_buffer_head + 1);
        ret = 1;
    }
    HAL_enable_irq(state);

    //return ret;
    (void)ret;
}

/*******************************************************************************
 * Function Name  : USB_USART_Flush_Data.
 * Description    : Flushes TX buffer
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void USB_USART_Flush_Data(void)
{
    while(USB_USART_Connected() && USB_USART_Available_Data_For_Write() != (USBD_MCDC.tx_buffer_size - 1));
    // We should also wait for USB_Tx_State to become 0, as hardware might still be busy transmitting data
    while(USBD_MCDC.tx_state == 1);
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
