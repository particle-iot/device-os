/**
  ******************************************************************************
  * @file    usb_prop.cpp
  * @author  Satish Nair
  * @version V1.0.0
  * @date    24-April-2013
  * @brief   All processing related to USB CDC-HID
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
#include "usb_conf.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "usb_pwr.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t Request = 0;

volatile LINE_CODING linecoding =
  {
    0x00,   /* baud rate*/
    0x00,   /* stop bits-1*/
    0x00,   /* parity - none*/
    0x00    /* no. of bits */
  };

static linecoding_bitrate_handler APP_LineCodingBitRateHandler = NULL;

volatile uint32_t ProtocolValue;

/* -------------------------------------------------------------------------- */
/*  Structures initializations */
/* -------------------------------------------------------------------------- */

const DEVICE Device_Table =
  {
    EP_NUM,
    1
  };

const DEVICE_PROP Device_Property =
  {
    USB_init,
    USB_Reset,
    USB_Status_In,
    USB_Status_Out,
    USB_Data_Setup,
    USB_NoData_Setup,
    USB_Get_Interface_Setting,
    USB_GetDeviceDescriptor,
    USB_GetConfigDescriptor,
    USB_GetStringDescriptor,
    0,
    0x40 /*MAX PACKET SIZE*/
  };

const USER_STANDARD_REQUESTS User_Standard_Requests =
  {
    USB_GetConfiguration,
    USB_SetConfiguration,
    USB_GetInterface,
    USB_SetInterface,
    USB_GetStatus,
    USB_ClearFeature,
    USB_SetEndPointFeature,
    USB_SetDeviceFeature,
    USB_SetDeviceAddress
  };

#ifdef USB_CDC_ENABLE
const ONE_DESCRIPTOR Device_Descriptor =
  {
    (uint8_t*)CDC_DeviceDescriptor,
    CDC_SIZ_DEVICE_DESC
  };

const ONE_DESCRIPTOR Config_Descriptor =
  {
    (uint8_t*)CDC_ConfigDescriptor,
    CDC_SIZ_CONFIG_DESC
  };
#endif

#ifdef USB_HID_ENABLE
const ONE_DESCRIPTOR Device_Descriptor =
  {
    (uint8_t*)HID_DeviceDescriptor,
    HID_SIZ_DEVICE_DESC
  };

const ONE_DESCRIPTOR Config_Descriptor =
  {
    (uint8_t*)HID_ConfigDescriptor,
    HID_SIZ_CONFIG_DESC
  };

const ONE_DESCRIPTOR HID_Report_Descriptor =
  {
    (uint8_t *)HID_ReportDescriptor,
    HID_SIZ_REPORT_DESC
  };

const ONE_DESCRIPTOR HID_Descriptor =
  {
    (uint8_t*)HID_ConfigDescriptor + HID_OFF_HID_DESC,
    HID_SIZ_HID_DESC
  };
#endif

const ONE_DESCRIPTOR String_Descriptor[4] =
  {
    {(uint8_t*)USB_StringLangID, USB_SIZ_STRING_LANGID},
    {(uint8_t*)USB_StringVendor, USB_SIZ_STRING_VENDOR},
    {(uint8_t*)USB_StringProduct, USB_SIZ_STRING_PRODUCT},
    {(uint8_t*)USB_StringSerial, USB_SIZ_STRING_SERIAL}
  };

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Extern function prototypes ------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

void SetLineCodingBitRateHandler(linecoding_bitrate_handler handler)
{
    APP_LineCodingBitRateHandler = handler;
}

/*******************************************************************************
* Function Name  : USB_init.
* Description    : USB init routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void USB_init(void)
{

  /* Update the serial number string descriptor with the data from the unique
  ID*/
  Get_SerialNum();

  pInformation->Current_Configuration = 0;

  /* Connect the device */
  PowerOn();

  /* Perform basic device initialization operations */
  USB_SIL_Init();

  bDeviceState = UNCONNECTED;
}

/*******************************************************************************
* Function Name  : USB_Reset
* Description    : USB reset routine
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void USB_Reset(void)
{
  /* Set USB DEVICE as not configured */
  pInformation->Current_Configuration = 0;

  /* Set USB DEVICE with the default Interface*/
  pInformation->Current_Interface = 0;

#ifdef USB_CDC_ENABLE
  /* Current Feature initialization */
  pInformation->Current_Feature = CDC_ConfigDescriptor[7];
#endif

#ifdef USB_HID_ENABLE
  /* Current Feature initialization */
  pInformation->Current_Feature = HID_ConfigDescriptor[7];
#endif

  SetBTABLE(BTABLE_ADDRESS);

  /* Initialize Endpoint 0 */
  SetEPType(ENDP0, EP_CONTROL);
  SetEPTxStatus(ENDP0, EP_TX_STALL);
  SetEPRxAddr(ENDP0, ENDP0_RXADDR);
  SetEPTxAddr(ENDP0, ENDP0_TXADDR);
  Clear_Status_Out(ENDP0);
  SetEPRxCount(ENDP0, Device_Property.MaxPacketSize);
  SetEPRxValid(ENDP0);

#ifdef USB_CDC_ENABLE
  /* Initialize Endpoint 1 */
  SetEPType(ENDP1, EP_BULK);
  SetEPTxAddr(ENDP1, ENDP1_TXADDR);
  SetEPTxStatus(ENDP1, EP_TX_NAK);
  SetEPRxStatus(ENDP1, EP_RX_DIS);

  /* Initialize Endpoint 2 */
  SetEPType(ENDP2, EP_INTERRUPT);
  SetEPTxAddr(ENDP2, ENDP2_TXADDR);
  SetEPRxStatus(ENDP2, EP_RX_DIS);
  SetEPTxStatus(ENDP2, EP_TX_NAK);

  /* Initialize Endpoint 3 */
  SetEPType(ENDP3, EP_BULK);
  SetEPRxAddr(ENDP3, ENDP3_RXADDR);
  SetEPRxCount(ENDP3, CDC_DATA_SIZE);
  SetEPRxStatus(ENDP3, EP_RX_VALID);
  SetEPTxStatus(ENDP3, EP_TX_DIS);
#endif

#ifdef USB_HID_ENABLE
  /* Initialize Endpoint 1 */
  SetEPType(ENDP1, EP_INTERRUPT);
  SetEPTxAddr(ENDP1, ENDP1_TXADDR);
#if defined (SPARK_USB_MOUSE)
  SetEPTxCount(ENDP1, 4);
#elif defined (SPARK_USB_KEYBOARD)
  SetEPTxCount(ENDP1, 8);
#endif
  SetEPRxStatus(ENDP1, EP_RX_DIS);
  SetEPTxStatus(ENDP1, EP_TX_NAK);
#endif

  /* Set this device to response on default address */
  SetDeviceAddress(0);

  bDeviceState = ATTACHED;
}

/*******************************************************************************
* Function Name  : USB_SetConfiguration.
* Description    : Update the device state to configured.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void USB_SetConfiguration(void)
{
  const DEVICE_INFO *pInfo = &Device_Info;

  if (pInfo->Current_Configuration != 0)
  {
    /* Device configured */
    bDeviceState = CONFIGURED;
  }
}

/*******************************************************************************
* Function Name  : USB_SetConfiguration.
* Description    : Update the device state to addressed.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void USB_SetDeviceAddress (void)
{
  bDeviceState = ADDRESSED;
}

/*******************************************************************************
* Function Name  : USB_Status_In.
* Description    : USB Status In Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void USB_Status_In(void)
{
#ifdef USB_CDC_ENABLE
  if (Request == SET_LINE_CODING)
  {
    //Callback handler when the host sets a specific linecoding
    if (NULL != APP_LineCodingBitRateHandler)
    {
      APP_LineCodingBitRateHandler(linecoding.bitrate);
    }

    Request = 0;
  }
#endif
}

/*******************************************************************************
* Function Name  : USB_Status_Out
* Description    : USB Status OUT Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void USB_Status_Out(void)
{
}

/*******************************************************************************
* Function Name  : USB_Data_Setup
* Description    : handle the data class specific requests
* Input          : Request Nb.
* Output         : None.
* Return         : USB_UNSUPPORT or USB_SUCCESS.
*******************************************************************************/
RESULT USB_Data_Setup(uint8_t RequestNo)
{
  uint8_t    *(*CopyRoutine)(uint16_t);

  CopyRoutine = NULL;

#ifdef USB_CDC_ENABLE
  if (RequestNo == GET_LINE_CODING)
  {
    if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
    {
      CopyRoutine = CDC_GetLineCoding;
    }
  }
  else if (RequestNo == SET_LINE_CODING)
  {
    if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
    {
      CopyRoutine = CDC_SetLineCoding;
    }
    Request = SET_LINE_CODING;
  }
#endif

#ifdef USB_HID_ENABLE
  if ((RequestNo == GET_DESCRIPTOR)
      && (Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT))
      && (pInformation->USBwIndex0 == 0))
  {
    if (pInformation->USBwValue1 == REPORT_DESCRIPTOR)
    {
      CopyRoutine = HID_GetReportDescriptor;
    }
    else if (pInformation->USBwValue1 == HID_DESCRIPTOR_TYPE)
    {
      CopyRoutine = HID_GetDescriptor;
    }

  } /* End of GET_DESCRIPTOR */

  /*** GET_PROTOCOL ***/
  else if ((Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
           && RequestNo == GET_PROTOCOL)
  {
    CopyRoutine = HID_GetProtocolValue;
  }
#endif

  if (CopyRoutine == NULL)
  {
    return USB_UNSUPPORT;
  }

  pInformation->Ctrl_Info.CopyData = CopyRoutine;
  pInformation->Ctrl_Info.Usb_wOffset = 0;
  (*CopyRoutine)(0);
  return USB_SUCCESS;
}

/*******************************************************************************
* Function Name  : USB_NoData_Setup.
* Description    : handle the no data class specific requests.
* Input          : Request Nb.
* Output         : None.
* Return         : USB_UNSUPPORT or USB_SUCCESS.
*******************************************************************************/
RESULT USB_NoData_Setup(uint8_t RequestNo)
{
#ifdef USB_CDC_ENABLE
  if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
  {
    if (RequestNo == SET_COMM_FEATURE)
    {
      return USB_SUCCESS;
    }
    else if (RequestNo == SET_CONTROL_LINE_STATE)
    {
      return USB_SUCCESS;
    }
  }
#endif

#ifdef USB_HID_ENABLE
  if ((Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
      && (RequestNo == SET_PROTOCOL))
  {
    return HID_SetProtocol();
  }
#endif

  return USB_UNSUPPORT;
}

/*******************************************************************************
* Function Name  : USB_GetDeviceDescriptor.
* Description    : Gets the device descriptor.
* Input          : Length.
* Output         : None.
* Return         : The address of the device descriptor.
*******************************************************************************/
uint8_t *USB_GetDeviceDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &Device_Descriptor);
}

/*******************************************************************************
* Function Name  : USB_GetConfigDescriptor.
* Description    : get the configuration descriptor.
* Input          : Length.
* Output         : None.
* Return         : The address of the configuration descriptor.
*******************************************************************************/
uint8_t *USB_GetConfigDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &Config_Descriptor);
}

/*******************************************************************************
* Function Name  : USB_GetStringDescriptor
* Description    : Gets the string descriptors according to the needed index
* Input          : Length.
* Output         : None.
* Return         : The address of the string descriptors.
*******************************************************************************/
uint8_t *USB_GetStringDescriptor(uint16_t Length)
{
  uint8_t wValue0 = pInformation->USBwValue0;
  if (wValue0 > 4)
  {
    return NULL;
  }
  else
  {
    return Standard_GetDescriptorData(Length, &String_Descriptor[wValue0]);
  }
}

/*******************************************************************************
* Function Name  : USB_Get_Interface_Setting.
* Description    : test the interface and the alternate setting according to the
*                  supported one.
* Input1         : uint8_t: Interface : interface number.
* Input2         : uint8_t: AlternateSetting : Alternate Setting number.
* Output         : None.
* Return         : The address of the string descriptors.
*******************************************************************************/
RESULT USB_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting)
{
  if (AlternateSetting > 0)
  {
    return USB_UNSUPPORT;
  }
  else if (Interface > 1)
  {
    return USB_UNSUPPORT;
  }

  return USB_SUCCESS;
}

#ifdef USB_CDC_ENABLE
/*******************************************************************************
* Function Name  : CDC_GetLineCoding.
* Description    : send the linecoding structure to the PC host.
* Input          : Length.
* Output         : None.
* Return         : Linecoding structure base address.
*******************************************************************************/
uint8_t *CDC_GetLineCoding(uint16_t Length)
{
  if (Length == 0)
  {
    pInformation->Ctrl_Info.Usb_wLength = sizeof(linecoding);
    return NULL;
  }

  return(uint8_t *)&linecoding;
}

/*******************************************************************************
* Function Name  : CDC_SetLineCoding.
* Description    : Set the linecoding structure fields.
* Input          : Length.
* Output         : None.
* Return         : Linecoding structure base address.
*******************************************************************************/
uint8_t *CDC_SetLineCoding(uint16_t Length)
{
  if (Length == 0)
  {
    pInformation->Ctrl_Info.Usb_wLength = sizeof(linecoding);
    return NULL;
  }

  return(uint8_t *)&linecoding;
}
#endif

#ifdef USB_HID_ENABLE
/*******************************************************************************
* Function Name  : HID_GetReportDescriptor.
* Description    : Gets the HID report descriptor.
* Input          : Length
* Output         : None.
* Return         : The address of the configuration descriptor.
*******************************************************************************/
uint8_t *HID_GetReportDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &HID_Report_Descriptor);
}

/*******************************************************************************
* Function Name  : HID_GetDescriptor.
* Description    : Gets the HID descriptor.
* Input          : Length
* Output         : None.
* Return         : The address of the configuration descriptor.
*******************************************************************************/
uint8_t *HID_GetDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &HID_Descriptor);
}

/*******************************************************************************
* Function Name  : HID_SetProtocol
* Description    : HID Set Protocol request routine.
* Input          : None.
* Output         : None.
* Return         : USB SUCCESS.
*******************************************************************************/
RESULT HID_SetProtocol(void)
{
  uint8_t wValue0 = pInformation->USBwValue0;
  ProtocolValue = wValue0;
  return USB_SUCCESS;
}

/*******************************************************************************
* Function Name  : HID_GetProtocolValue
* Description    : get the protocol value
* Input          : Length.
* Output         : None.
* Return         : address of the protocol value.
*******************************************************************************/
uint8_t *HID_GetProtocolValue(uint16_t Length)
{
  if (Length == 0)
  {
    pInformation->Ctrl_Info.Usb_wLength = 1;
    return NULL;
  }
  else
  {
    return (uint8_t *)(&ProtocolValue);
  }
}
#endif
