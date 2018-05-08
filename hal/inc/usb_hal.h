/**
 ******************************************************************************
 * @file    usb_hal.h
 * @author  Satish Nair
 * @version V1.0.0
 * @date    10-Sept-2014
 * @brief   USB Virtual COM Port and HID device HAL
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_HAL_H
#define __USB_HAL_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macros ------------------------------------------------------------*/

#include "usb_config_hal.h"

#ifdef USE_STDPERIPH_DRIVER
// this is one way to determine if the platform module is being used or not
#include "hw_config.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USB_VENDOR_REQUEST_ENABLE
typedef struct HAL_USB_SetupRequest {
  union {
    uint8_t bmRequestType;
    struct {
      uint8_t bmRequestTypeRecipient : 5;
      uint8_t bmRequestTypeType : 2;
      uint8_t bmRequestTypeDirection : 1;
    };
  };
  uint8_t bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
  uint8_t* data;
} HAL_USB_SetupRequest;

typedef enum {
  HAL_USB_VENDOR_REQUEST_STATE_NONE = 0,
  /* HAL_USB_VENDOR_REQUEST_STATE_TX_COMPLETED state indicates that the data stage of
   * a Device->Host request has completed and the buffer that was supplied by the vendor
   * request callback can be safely freed.
   */
  HAL_USB_VENDOR_REQUEST_STATE_TX_COMPLETED = 1,
  /* HAL_USB_VENDOR_REQUEST_STATE_RESET state indicates that any resources used for handling
   * vendor requests may be safely freed, as either the host has disconnected or sent a
   * reset command on the bus.
   */
  HAL_USB_VENDOR_REQUEST_STATE_RESET = 2
} HAL_USB_VendorRequestState;

typedef uint8_t (*HAL_USB_Vendor_Request_Callback)(HAL_USB_SetupRequest* req, void* p);
typedef uint8_t (*HAL_USB_Vendor_Request_State_Callback)(HAL_USB_VendorRequestState state, void* p);
#endif // USB_VENDOR_REQUEST_ENABLE

    /* USB Config : IMR_MSK */
/* mask defining which events has to be handled */
/* by the device application software */
#define IMR_MSK (CNTR_CTRM  | \
                 CNTR_WKUPM | \
                 CNTR_SUSPM | \
                 CNTR_ERRM  | \
                 CNTR_SOFM  | \
                 CNTR_ESOFM | \
                 CNTR_RESETM  \
)

/* Exported functions ------------------------------------------------------- */
#if defined (USB_CDC_ENABLE) || defined (USB_HID_ENABLE)
void SPARK_USB_Setup(void);
void Get_SerialNum(void);
#endif

#ifdef USB_CDC_ENABLE
/**
 * Initialize or deinitialize USB serial
 * @param baudRate  The data rate of the connection. If 0, the connection is
 * uninitialized.
 */
void USB_USART_Init(uint32_t baudRate);

unsigned int USB_USART_Baud_Rate(void);

void USB_USART_LineCoding_BitRate_Handler(void (*handler)(uint32_t bitRate));

/**
 * Retrieves the number of bytes of data available.
 * @return
 */
uint8_t USB_USART_Available_Data(void);

/**
 * Retrieves the number of bytes of data available in the TX buffer.
 * @return
 */
int32_t USB_USART_Available_Data_For_Write(void);

/**
 * Reads data from the input buffer.
 * @param peek  If the data should be peeked reather than fetched.
 * The default, `false` means fetch, where data is removed from the buffer.
 * When `true`, the data byte is left in the buffer.
 * @return
 */
int32_t USB_USART_Receive_Data(uint8_t peek);

/**
 * Sends data to the USB serial.
 * @param Data      The data to write.
 * @return
 */
void USB_USART_Send_Data(uint8_t Data);

/**
 * Flushes TX buffer
 * @return
 */
void USB_USART_Flush_Data(void);

#endif

#ifdef USB_HID_ENABLE
void USB_HID_Send_Report(void *pHIDReport, uint16_t reportSize);
#endif

/*******************************************************************************************************/
/* Multi-instanced USB classes */
/*******************************************************************************************************/

#ifdef USB_VENDOR_REQUEST_ENABLE
void HAL_USB_Set_Vendor_Request_Callback(HAL_USB_Vendor_Request_Callback cb, void* p);
void HAL_USB_Set_Vendor_Request_State_Callback(HAL_USB_Vendor_Request_State_Callback cb, void* p);
#endif

#if defined(USB_CDC_ENABLE) || defined(USB_HID_ENABLE)
void HAL_USB_Init();
void HAL_USB_Attach();
void HAL_USB_Detach();
#endif

#ifdef USB_CDC_ENABLE

typedef enum HAL_USB_USART_Serial {
  HAL_USB_USART_SERIAL = 0,
  HAL_USB_USART_SERIAL1 = 1,

  HAL_USB_USART_SERIAL_COUNT
} HAL_USB_USART_Serial;

typedef struct HAL_USB_USART_Config {
  uint16_t size;
  uint8_t* rx_buffer;
  uint16_t rx_buffer_size;
  uint8_t* tx_buffer;
  uint16_t tx_buffer_size;
} HAL_USB_USART_Config;

void HAL_USB_USART_Init(HAL_USB_USART_Serial serial, const HAL_USB_USART_Config* config);
void HAL_USB_USART_Begin(HAL_USB_USART_Serial serial, uint32_t baud, void *reserved);
void HAL_USB_USART_End(HAL_USB_USART_Serial serial);
unsigned int HAL_USB_USART_Baud_Rate(HAL_USB_USART_Serial serial);
int32_t HAL_USB_USART_Available_Data(HAL_USB_USART_Serial serial);
int32_t HAL_USB_USART_Available_Data_For_Write(HAL_USB_USART_Serial serial);
int32_t HAL_USB_USART_Receive_Data(HAL_USB_USART_Serial serial, uint8_t peek);
int32_t HAL_USB_USART_Send_Data(HAL_USB_USART_Serial serial, uint8_t data);
void HAL_USB_USART_Flush_Data(HAL_USB_USART_Serial serial);
bool HAL_USB_USART_Is_Enabled(HAL_USB_USART_Serial serial);
bool HAL_USB_USART_Is_Connected(HAL_USB_USART_Serial serial);
int32_t HAL_USB_USART_LineCoding_BitRate_Handler(void (*handler)(uint32_t bitRate), void* reserved);
#endif

#ifdef USB_HID_ENABLE
void HAL_USB_HID_Init(uint8_t reserved, void* reserved1);
void HAL_USB_HID_Begin(uint8_t reserved, void* reserved1);
void HAL_USB_HID_Send_Report(uint8_t reserved, void *pHIDReport, uint16_t reportSize, void* reserved1);
int32_t HAL_USB_HID_Status(uint8_t reserved, void* reserved1);
void HAL_USB_HID_End(uint8_t reserved);
uint8_t HAL_USB_HID_Set_State(uint8_t id, uint8_t state, void* reserved);
#endif


// MDM: I'm putting these here so we can export them from the hal_usb dynalib
/* USB Interrupt Handlers from usb_hal.c */
#ifdef USE_USB_OTG_FS
    extern void OTG_FS_WKUP_irq(void);
    extern void OTG_FS_irq(void);
#elif defined USE_USB_OTG_HS
    extern void OTG_HS_EP1_OUT_irq(void);
    extern void OTG_HS_EP1_IN_irq(void);
    extern void OTG_HS_WKUP_irq(void);
    extern void OTG_HS_irq(void);
#endif


#ifdef __cplusplus
}
#endif


#endif  /* __USB_HAL_H */
