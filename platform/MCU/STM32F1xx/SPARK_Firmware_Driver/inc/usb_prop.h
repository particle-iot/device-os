/**
  ******************************************************************************
  * @file    usb_prop.h
  * @author  Satish Nair
  * @version V1.0.0
  * @date    24-April-2013
  * @brief   All processing related to USB CDC-HID (Endpoint 0)
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
#ifndef __usb_prop_H
#define __usb_prop_H

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct
{
  uint32_t bitrate;
  uint8_t format;
  uint8_t paritytype;
  uint8_t datatype;
} LINE_CODING;

typedef enum _HID_REQUESTS
{
  GET_REPORT = 1,
  GET_IDLE,
  GET_PROTOCOL,

  SET_REPORT = 9,
  SET_IDLE,
  SET_PROTOCOL
} HID_REQUESTS;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported define -----------------------------------------------------------*/

#define USB_GetConfiguration          NOP_Process
//#define USB_SetConfiguration          NOP_Process
#define USB_GetInterface              NOP_Process
#define USB_SetInterface              NOP_Process
#define USB_GetStatus                 NOP_Process
#define USB_ClearFeature              NOP_Process
#define USB_SetEndPointFeature        NOP_Process
#define USB_SetDeviceFeature          NOP_Process
//#define USB_SetDeviceAddress          NOP_Process

#define SEND_ENCAPSULATED_COMMAND     0x00
#define GET_ENCAPSULATED_RESPONSE     0x01
#define SET_COMM_FEATURE              0x02
#define GET_COMM_FEATURE              0x03
#define CLEAR_COMM_FEATURE            0x04
#define SET_LINE_CODING               0x20
#define GET_LINE_CODING               0x21
#define SET_CONTROL_LINE_STATE        0x22
#define SEND_BREAK                    0x23

#define REPORT_DESCRIPTOR             0x22

/* Exported functions ------------------------------------------------------- */
void USB_init(void);
void USB_Reset(void);
void USB_SetConfiguration(void);
void USB_SetDeviceAddress (void);
void USB_Status_In (void);
void USB_Status_Out (void);
RESULT USB_Data_Setup(uint8_t);
RESULT USB_NoData_Setup(uint8_t);
RESULT USB_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting);
uint8_t *USB_GetDeviceDescriptor(uint16_t );
uint8_t *USB_GetConfigDescriptor(uint16_t);
uint8_t *USB_GetStringDescriptor(uint16_t);

#ifdef USB_CDC_ENABLE
uint8_t *CDC_GetLineCoding(uint16_t Length);
uint8_t *CDC_SetLineCoding(uint16_t Length);
#endif

#ifdef USB_HID_ENABLE
uint8_t *HID_GetReportDescriptor(uint16_t Length);
uint8_t *HID_GetDescriptor(uint16_t Length);
RESULT HID_SetProtocol(void);
uint8_t *HID_GetProtocolValue(uint16_t Length);
#endif

typedef void (*linecoding_bitrate_handler)(uint32_t bitrate);
void SetLineCodingBitRateHandler(linecoding_bitrate_handler handler);

#endif /* __usb_prop_H */

