/**
 ******************************************************************************
 * @file    usbd_desc.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    21-Oct-2014
 * @brief   This file provides the USBD descriptors and string formating method.
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  Copyright 2012 STMicroelectronics
  http://www.st.com/software_license_agreement_liberty_v2

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
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_req.h"
#include "usb_regs.h"

#define USBD_LANGID_STRING              0x0409  //U.S. English
#define USBD_MANUFACTURER_STRING        "Particle"

#define USBD_PRODUCT_HS_STRING          "Photon DFU Mode"
#define USBD_SERIALNUMBER_HS_STRING     "00000000010B"

#define USBD_PRODUCT_FS_STRING          "Photon DFU Mode"
#define USBD_SERIALNUMBER_FS_STRING     "00000000010C"

#define USBD_CONFIGURATION_HS_STRING    "DFU Config"
#define USBD_INTERFACE_HS_STRING        "DFU Interface"

#define USBD_CONFIGURATION_FS_STRING    "DFU Config"
#define USBD_INTERFACE_FS_STRING        "DFU Interface"

USBD_DEVICE USR_desc =
{
        USBD_USR_DeviceDescriptor,
        USBD_USR_LangIDStrDescriptor,
        USBD_USR_ManufacturerStrDescriptor,
        USBD_USR_ProductStrDescriptor,
        USBD_USR_SerialStrDescriptor,
        USBD_USR_ConfigStrDescriptor,
        USBD_USR_InterfaceStrDescriptor,
};

/* USB Standard Device Descriptor */
uint8_t USBD_DeviceDesc[USB_SIZ_DEVICE_DESC] =
{
        0x12,                       /*bLength */
        USB_DEVICE_DESCRIPTOR_TYPE, /*bDescriptorType*/
        0x00,                       /*bcdUSB */
        0x02,
        0x00,                       /*bDeviceClass*/
        0x00,                       /*bDeviceSubClass*/
        0x00,                       /*bDeviceProtocol*/
        USB_OTG_MAX_EP0_SIZE,       /*bMaxPacketSize*/
        LOBYTE(USBD_VID_SPARK),     /*idVendor*/
        HIBYTE(USBD_VID_SPARK),     /*idVendor*/
        LOBYTE(USBD_PID_DFU),       /*idProduct*/
        HIBYTE(USBD_PID_DFU),       /*idProduct*/
        0x00,                       /*bcdDevice rel. 2.00*/
        0x02,
        USBD_IDX_MFC_STR,           /*Index of manufacturer  string*/
        USBD_IDX_PRODUCT_STR,       /*Index of product string*/
        USBD_IDX_SERIAL_STR,        /*Index of serial number string*/
        USBD_CFG_MAX_NUM            /*bNumConfigurations*/
} ; /* USB_DeviceDescriptor */

/* USB Standard Device Descriptor */
uint8_t USBD_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] =
{
        USB_LEN_DEV_QUALIFIER_DESC,
        USB_DESC_TYPE_DEVICE_QUALIFIER,
        0x00,
        0x02,
        0x00,
        0x00,
        0x00,
        0x40,
        0x01,
        0x00,
};

/* USB Standard Device Descriptor */
uint8_t USBD_LangIDDesc[USB_SIZ_STRING_LANGID] =
{
        USB_SIZ_STRING_LANGID,
        USB_DESC_TYPE_STRING,
        LOBYTE(USBD_LANGID_STRING),
        HIBYTE(USBD_LANGID_STRING),
};

/**
 * @brief  USBD_USR_DeviceDescriptor
 *         return the device descriptor
 * @param  speed : current device speed
 * @param  length : pointer to data length variable
 * @retval pointer to descriptor buffer
 */
uint8_t *  USBD_USR_DeviceDescriptor( uint8_t speed , uint16_t *length)
{
    *length = sizeof(USBD_DeviceDesc);
    return USBD_DeviceDesc;
}

/**
 * @brief  USBD_USR_LangIDStrDescriptor
 *         return the LangID string descriptor
 * @param  speed : current device speed
 * @param  length : pointer to data length variable
 * @retval pointer to descriptor buffer
 */
uint8_t *  USBD_USR_LangIDStrDescriptor( uint8_t speed , uint16_t *length)
{
    *length = sizeof(USBD_LangIDDesc);
    return USBD_LangIDDesc;
}

/**
 * @brief  USBD_USR_ProductStrDescriptor
 *         return the product string descriptor
 * @param  speed : current device speed
 * @param  length : pointer to data length variable
 * @retval pointer to descriptor buffer
 */
uint8_t *  USBD_USR_ProductStrDescriptor( uint8_t speed , uint16_t *length)
{
    if(speed == USB_OTG_SPEED_HIGH)
    {
        USBD_GetString (USBD_PRODUCT_HS_STRING, USBD_StrDesc, length);
    }
    else
    {
        USBD_GetString (USBD_PRODUCT_FS_STRING, USBD_StrDesc, length);
    }
    return USBD_StrDesc;
}

/**
 * @brief  USBD_USR_ManufacturerStrDescriptor
 *         return the manufacturer string descriptor
 * @param  speed : current device speed
 * @param  length : pointer to data length variable
 * @retval pointer to descriptor buffer
 */
uint8_t *  USBD_USR_ManufacturerStrDescriptor( uint8_t speed , uint16_t *length)
{
    USBD_GetString (USBD_MANUFACTURER_STRING, USBD_StrDesc, length);
    return USBD_StrDesc;
}

/**
 * @brief  USBD_USR_SerialStrDescriptor
 *         return the serial number string descriptor
 * @param  speed : current device speed
 * @param  length : pointer to data length variable
 * @retval pointer to descriptor buffer
 */
uint8_t *  USBD_USR_SerialStrDescriptor( uint8_t speed , uint16_t *length)
{
    if(speed == USB_OTG_SPEED_HIGH)
    {
        USBD_GetString (USBD_SERIALNUMBER_HS_STRING, USBD_StrDesc, length);
    }
    else
    {
        USBD_GetString (USBD_SERIALNUMBER_FS_STRING, USBD_StrDesc, length);
    }
    return USBD_StrDesc;
}

/**
 * @brief  USBD_USR_ConfigStrDescriptor
 *         return the configuration string descriptor
 * @param  speed : current device speed
 * @param  length : pointer to data length variable
 * @retval pointer to descriptor buffer
 */
uint8_t *  USBD_USR_ConfigStrDescriptor( uint8_t speed , uint16_t *length)
{
    if(speed == USB_OTG_SPEED_HIGH)
    {
        USBD_GetString (USBD_CONFIGURATION_HS_STRING, USBD_StrDesc, length);
    }
    else
    {
        USBD_GetString (USBD_CONFIGURATION_FS_STRING, USBD_StrDesc, length);
    }
    return USBD_StrDesc;
}

/**
 * @brief  USBD_USR_InterfaceStrDescriptor
 *         return the interface string descriptor
 * @param  speed : current device speed
 * @param  length : pointer to data length variable
 * @retval pointer to descriptor buffer
 */
uint8_t *  USBD_USR_InterfaceStrDescriptor( uint8_t speed , uint16_t *length)
{
    if(speed == USB_OTG_SPEED_HIGH)
    {
        USBD_GetString (USBD_INTERFACE_HS_STRING, USBD_StrDesc, length);
    }
    else
    {
        USBD_GetString (USBD_INTERFACE_FS_STRING, USBD_StrDesc, length);
    }
    return USBD_StrDesc;
}
