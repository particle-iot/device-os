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

extern void *malloc(uint32_t size);

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

static void (*LineCoding_BitRate_Handler)(uint32_t bitRate) = NULL;
static uint8_t USB_Configured = 0;

static uint8_t USB_SetupRequest_Data[HAL_USB_SETUP_REQUEST_MAX_DATA];
static HAL_USB_SetupRequest USB_SetupRequest = {{0}};
static uint8_t USB_InSetupRequest = 0;
static HAL_USB_Vendor_Request_Callback USB_Vendor_Request_Callback = NULL;

#ifdef USB_VENDOR_REQUEST_ENABLE

void HAL_USB_Set_Vendor_Request_Callback(HAL_USB_Vendor_Request_Callback cb, void* reserved)
{
    USB_Vendor_Request_Callback = cb;
}

uint8_t HAL_USB_Handle_Vendor_Request(USB_SETUP_REQ* req, uint8_t dataStage)
{
    uint8_t ret = USBD_OK;
    if (req != NULL) {
        USB_SetupRequest.bmRequestType = req->bmRequest;
        USB_SetupRequest.bRequest = req->bRequest;
        USB_SetupRequest.wValue = req->wValue;
        USB_SetupRequest.wIndex = req->wIndex;
        USB_SetupRequest.wLength = req->wLength;

        if (req->wLength) {
            // Setup request with data stage

            if (req->bmRequest & 0x80) {
                // Device -> Host
                if (USB_Vendor_Request_Callback) {
                    USB_SetupRequest.data = USB_SetupRequest_Data;
                    ret = USB_Vendor_Request_Callback(&USB_SetupRequest);

                    if (ret == USBD_OK && USB_SetupRequest.data != NULL &&
                        USB_SetupRequest.wLength && USB_SetupRequest.wLength <= HAL_USB_SETUP_REQUEST_MAX_DATA) {
                        if (USB_SetupRequest.data != USB_SetupRequest_Data) {
                            memcpy(USB_SetupRequest_Data, USB_SetupRequest.data, USB_SetupRequest.wLength);
                        }
                        USBD_CtlSendData (&USB_OTG_dev, USB_SetupRequest_Data, USB_SetupRequest.wLength);
                    } else {
                        ret = USBD_FAIL;
                    }
                }
            } else {
                // Host -> Device
                USB_InSetupRequest = 1;
                USBD_CtlPrepareRx(&USB_OTG_dev, USB_SetupRequest_Data, req->wLength);
            }
        } else {
            // Setup request without data stage
            if (USB_Vendor_Request_Callback) {
                USB_SetupRequest.data = NULL;
                ret = USB_Vendor_Request_Callback(&USB_SetupRequest);
            }
        }
    } else if (dataStage && USB_InSetupRequest) {
        USB_InSetupRequest = 0;
        if (USB_Vendor_Request_Callback) {
            USB_SetupRequest.data = USB_SetupRequest_Data;
            ret = USB_Vendor_Request_Callback(&USB_SetupRequest);
        }
    } else {
        ret = USBD_FAIL;
    }

    return ret;
}

#endif // USB_VENDOR_REQUEST_ENABLE

#if defined (USB_CDC_ENABLE) || defined (USB_HID_ENABLE)
/*******************************************************************************
 * Function Name  : SPARK_USB_Setup
 * Description    : Spark USB Setup.
 * Input          : None.
 * Return         : None.
 *******************************************************************************/
void SPARK_USB_Setup(void)
{
    if (!USB_Configured)
        return;

    USBD_Init(&USB_OTG_dev,
#ifdef USE_USB_OTG_FS
            USB_OTG_FS_CORE_ID,
#elif defined USE_USB_OTG_HS
            USB_OTG_HS_CORE_ID,
#endif
            &USR_desc,
            //&USBD_CDC_cb,
            &USBD_Composite_cb,
            &USR_cb);
    HAL_USB_Attach();
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

void USB_USART_LineCoding_BitRate_Handler(void (*handler)(uint32_t bitRate))
{
    // Enable Serial by default
    HAL_USB_USART_Begin(HAL_USB_USART_SERIAL, 9600, NULL);
    LineCoding_BitRate_Handler = handler;
}

uint16_t HAL_USB_USART_Request_Handler(USBD_Composite_Class_Data* cls, uint32_t cmd, uint8_t* buf, uint32_t len) {
    if (cmd == SET_LINE_CODING && LineCoding_BitRate_Handler) {
        USBD_MCDC_Instance_Data* priv = (USBD_MCDC_Instance_Data*)cls->priv;
        if (priv)
            LineCoding_BitRate_Handler(priv->linecoding.bitrate);
    }

    return 0;
}

typedef struct HAL_USB_USART_Instance_Info
{
    uint8_t registered;
    void* cls;
    USBD_MCDC_Instance_Data* data;
} HAL_USB_USART_Info;

static USBD_MCDC_Instance_Data usbUsartInstanceData[HAL_USB_USART_SERIAL_COUNT] = { {0} };
static HAL_USB_USART_Info usbUsartMap[HAL_USB_USART_SERIAL_COUNT] = { {0} };

// static void HAL_USB_SoftReattach(void)
// {
//     USB_OTG_dev.regs.DREGS->DCTL |= 0x02;
// }

void HAL_USB_Init(void)
{
    if (USB_Configured)
        return;
    // Pre-register Serial and USBSerial1 but keep them inactive.
    // This is only needed to reserve interfaces #0 and #1 for Serial, and #2 and #3 for USBSerial1
    HAL_USB_USART_Init(HAL_USB_USART_SERIAL, NULL);
    HAL_USB_USART_Init(HAL_USB_USART_SERIAL1, NULL);

    USB_Configured = 1;
    SPARK_USB_Setup();
}

void HAL_USB_Detach(void)
{
    if (USB_Configured)
        USB_Cable_Config(DISABLE);
}

void HAL_USB_Attach(void)
{
    if (USB_Configured) {
        // Do not attach if there are no USB classes registered
        if (USBD_Composite_Registered_Count(true) > 0)
            USB_Cable_Config(ENABLE);
    }
}

void HAL_USB_USART_Init(HAL_USB_USART_Serial serial, const HAL_USB_USART_Config* config)
{
    usbUsartMap[serial].data = &usbUsartInstanceData[serial];

    if (serial == HAL_USB_USART_SERIAL) {
        usbUsartMap[serial].data->ep_in_data = CDC0_IN_EP;
        usbUsartMap[serial].data->ep_in_int = CDC0_CMD_EP;
        usbUsartMap[serial].data->ep_out_data = CDC0_OUT_EP;
        usbUsartMap[serial].data->name = USBD_PRODUCT_STRING " " "Serial";
    } else if (serial == HAL_USB_USART_SERIAL1) {
        usbUsartMap[serial].data->ep_in_data = CDC1_IN_EP;
        usbUsartMap[serial].data->ep_in_int = CDC1_CMD_EP;
        usbUsartMap[serial].data->ep_out_data = CDC1_OUT_EP;
        usbUsartMap[serial].data->name = USBD_PRODUCT_STRING " " "USBSerial1";
    }

    if (config) {
        // Just in case disable interrupts
        int32_t state = HAL_disable_irq();
        usbUsartMap[serial].data->rx_buffer = config->rx_buffer;
        usbUsartMap[serial].data->rx_buffer_size = config->rx_buffer_size;

        usbUsartMap[serial].data->tx_buffer = config->tx_buffer;
        usbUsartMap[serial].data->tx_buffer_size = config->tx_buffer_size;
        HAL_enable_irq(state);
    }

    usbUsartMap[serial].data->req_handler = HAL_USB_USART_Request_Handler;

    if (!usbUsartMap[serial].cls) {
        usbUsartMap[serial].cls = USBD_Composite_Register(&USBD_MCDC_cb, usbUsartMap[serial].data, serial == HAL_USB_USART_SERIAL ? 1 : 0);
        usbUsartMap[serial].data->cls = usbUsartMap[serial].cls;
        USBD_Composite_Set_State(usbUsartMap[serial].cls, false);
    }
}

void HAL_USB_USART_Begin(HAL_USB_USART_Serial serial, uint32_t baud, void *reserved)
{
    if (!usbUsartMap[serial].data || !usbUsartMap[serial].cls) {
        HAL_USB_USART_Init(serial, NULL);
    }
    
    if (!usbUsartMap[serial].registered) {
        if (USBD_Composite_Get_State(usbUsartMap[serial].cls) == false) {
            // Detach, change state, re-attach
            HAL_USB_Detach();
            USBD_Composite_Set_State(usbUsartMap[serial].cls, true);
            usbUsartMap[serial].registered = 1;
            usbUsartMap[serial].data->linecoding.bitrate = baud;
            HAL_USB_Attach();
        }
    }
}

void HAL_USB_USART_End(HAL_USB_USART_Serial serial)
{
    if (usbUsartMap[serial].registered) {
        HAL_USB_Detach();
        // Do not unregister, just deactivate to keep Serial and USBSerial1 using the same interface numbers
        // USBD_Composite_Unregister(usbUsartMap[serial].cls, usbUsartMap[serial].data);
        USBD_Composite_Set_State(usbUsartMap[serial].cls, false);
        usbUsartMap[serial].registered = 0;
        HAL_USB_Attach();
    }
}

unsigned int HAL_USB_USART_Baud_Rate(HAL_USB_USART_Serial serial)
{
    return usbUsartMap[serial].data->linecoding.bitrate;
}

int32_t HAL_USB_USART_Available_Data(HAL_USB_USART_Serial serial)
{
    int32_t available = 0;
    int state = HAL_disable_irq();
    if (usbUsartMap[serial].data->rx_buffer_head >= usbUsartMap[serial].data->rx_buffer_tail)
        available = usbUsartMap[serial].data->rx_buffer_head - usbUsartMap[serial].data->rx_buffer_tail;
    else
        available = usbUsartMap[serial].data->rx_buffer_length + usbUsartMap[serial].data->rx_buffer_head - usbUsartMap[serial].data->rx_buffer_tail;
    HAL_enable_irq(state);
    return available;
}

int32_t HAL_USB_USART_Available_Data_For_Write(HAL_USB_USART_Serial serial)
{
    if (HAL_USB_USART_Is_Connected(serial))
    {
        uint32_t tail = usbUsartMap[serial].data->tx_buffer_tail;
        int32_t available = usbUsartMap[serial].data->tx_buffer_size - (usbUsartMap[serial].data->tx_buffer_head >= tail ?
            usbUsartMap[serial].data->tx_buffer_head - tail : usbUsartMap[serial].data->tx_buffer_size + usbUsartMap[serial].data->tx_buffer_head - tail) - 1;
        return available;
    }

    return -1;
}

int32_t HAL_USB_USART_Receive_Data(HAL_USB_USART_Serial serial, uint8_t peek)
{
    if (HAL_USB_USART_Available_Data(serial) > 0)
    {
        int state = HAL_disable_irq();
        uint8_t data = usbUsartMap[serial].data->rx_buffer[usbUsartMap[serial].data->rx_buffer_tail];
        if (!peek) {
            usbUsartMap[serial].data->rx_buffer_tail++;
            if (usbUsartMap[serial].data->rx_buffer_tail == usbUsartMap[serial].data->rx_buffer_length)
                usbUsartMap[serial].data->rx_buffer_tail = 0;
        }
        HAL_enable_irq(state);
        return data;
    }

    return -1;
}

static bool HAL_USB_WillPreempt()
{
    if (HAL_IsISR()) {
#ifdef USE_USB_OTG_FS
        int32_t irq = OTG_FS_IRQn;
#else
        int32_t irq = OTG_HS_IRQn;
#endif
        if (!HAL_WillPreempt(irq, HAL_ServicedIRQn()))
            return false;
    }

    return true;
}

int32_t HAL_USB_USART_Send_Data(HAL_USB_USART_Serial serial, uint8_t data)
{
    int32_t available = 0;
    do {
        available = HAL_USB_USART_Available_Data_For_Write(serial);
    }
    while (available < 1 && available != -1 && HAL_USB_WillPreempt());
    // Confirm once again that the Host is connected
    if (HAL_USB_USART_Is_Connected(serial) && available > 0)
    {
        uint32_t head = usbUsartMap[serial].data->tx_buffer_head;

        usbUsartMap[serial].data->tx_buffer[head] = data;

        usbUsartMap[serial].data->tx_buffer_head = ++head % usbUsartMap[serial].data->tx_buffer_size;

        return 1;
    }

    return -1;
}

void HAL_USB_USART_Flush_Data(HAL_USB_USART_Serial serial)
{
    if (!HAL_USB_WillPreempt())
        return;
    while(HAL_USB_USART_Is_Connected(serial) && HAL_USB_USART_Available_Data_For_Write(serial) != (usbUsartMap[serial].data->tx_buffer_size - 1));
    // We should also wait for USB_Tx_State to become 0, as hardware might still be busy transmitting data
    while(usbUsartMap[serial].data->tx_state == 1);
}

bool HAL_USB_USART_Is_Enabled(HAL_USB_USART_Serial serial)
{
    return usbUsartMap[serial].registered;
}

bool HAL_USB_USART_Is_Connected(HAL_USB_USART_Serial serial)
{
    return usbUsartMap[serial].registered && usbUsartMap[serial].cls &&
                usbUsartMap[serial].data->linecoding.bitrate > 0 &&
                USB_OTG_dev.dev.device_status == USB_OTG_CONFIGURED &&
                usbUsartMap[serial].data->serial_open;
}
#endif /* USB_CDC_ENABLE */

#ifdef USB_HID_ENABLE

typedef struct HAL_USB_HID_Instance_Info {
    uint8_t configured;
    uint8_t registered;
    uint8_t refcount;
    void* cls;
} HAL_USB_HID_Instance_Info;

static HAL_USB_HID_Instance_Info usbHid = {0};

void HAL_USB_HID_Init(uint8_t reserved, void* reserved1)
{
    if (!usbHid.configured) {
        // Just in case run HAL_USB_Init() here to ensure that Serial and USBSerial1 were pre-registered
        HAL_USB_Init();
        usbHid.configured = 1;
    }
}

void HAL_USB_HID_Begin(uint8_t reserved, void* reserved1)
{
    if (usbHid.refcount == 0 && !usbHid.registered)
    {
        HAL_USB_Detach();
        usbHid.cls = USBD_Composite_Register(&USBD_MHID_cb, NULL, 0);
        usbHid.registered = 1;
        usbHid.refcount++;
        HAL_USB_Attach();
    } else {
        usbHid.refcount++;
    }
}

void HAL_USB_HID_End(uint8_t reserved)
{
    if (usbHid.registered && --usbHid.refcount == 0) {
        HAL_USB_Detach();
        USBD_Composite_Unregister(usbHid.cls, NULL);
        usbHid.registered = 0;
        HAL_USB_Attach();
    }
}

/*******************************************************************************
 * Function Name : USB_HID_Send_Report.
 * Description   : Send HID Report Info to Host.
 * Input         : pHIDReport and reportSize.
 * Output        : None.
 * Return value  : None.
 *******************************************************************************/
void HAL_USB_HID_Send_Report(uint8_t reserved, void *pHIDReport, uint16_t reportSize, void* reserved1)
{
    USBD_MHID_SendReport(&USB_OTG_dev, NULL, pHIDReport, reportSize);
}
#endif



/************************************************************************************************************/
/* Compatibility */
/************************************************************************************************************/
#ifdef USB_CDC_ENABLE
void USB_USART_Init(uint32_t baudRate)
{
    if (usbUsartMap[HAL_USB_USART_SERIAL].data == NULL) {
        // For compatibility we allocate buffers from heap here, as application calling USB_USART_Init
        // assumes that the driver has its own buffers
        HAL_USB_USART_Config conf;
        conf.rx_buffer = malloc(USB_RX_BUFFER_SIZE);
        conf.rx_buffer_size = USB_RX_BUFFER_SIZE;
        conf.tx_buffer = malloc(USB_TX_BUFFER_SIZE);
        conf.tx_buffer_size = USB_TX_BUFFER_SIZE;
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
    return HAL_USB_USART_Available_Data(HAL_USB_USART_SERIAL);
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
