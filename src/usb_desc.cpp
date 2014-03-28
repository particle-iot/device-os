/**
  ******************************************************************************
  * @file    usb_desc.cpp
  * @author  Satish Nair
  * @version V1.0.0
  * @date    24-April-2013
  * @brief   Descriptors for USB VCP-HID
  ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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
extern "C" {
#include "usb_conf.h"
#include "usb_lib.h"
#include "usb_desc.h"
}

#ifdef USB_VCP_ENABLE
/* USB Standard Device Descriptor */
const uint8_t VCP_DeviceDescriptor[] =
  {
    0x12,   /* bLength */
    USB_DEVICE_DESCRIPTOR_TYPE,     /* bDescriptorType */
    0x00,
    0x02,   /* bcdUSB = 2.00 */
    0x02,   /* bDeviceClass: CDC */
    0x00,   /* bDeviceSubClass */
    0x00,   /* bDeviceProtocol */
    0x40,   /* bMaxPacketSize0 */
    0x50,
    0x1D,   /* idVendor = 0x1D50 */
    0x7D,
    0x60,   /* idProduct = 0x607D */
    0x00,
    0x02,   /* bcdDevice = 2.00 */
    1,      /* Index of string descriptor describing manufacturer */
    2,      /* Index of string descriptor describing product */
    3,      /* Index of string descriptor describing the device's serial number */
    0x01    /* bNumConfigurations */
  };

const uint8_t VCP_ConfigDescriptor[] =
  {
    /*Configuration Descriptor*/
    0x09,   /* bLength: Configuration Descriptor size */
    USB_CONFIGURATION_DESCRIPTOR_TYPE,      /* bDescriptorType: Configuration */
    VCP_SIZ_CONFIG_DESC,       /* wTotalLength:no of returned bytes */
    0x00,
    0x02,   /* bNumInterfaces: 2 interface */
    0x01,   /* bConfigurationValue: Configuration value */
    0x00,   /* iConfiguration: Index of string descriptor describing the configuration */
    0xC0,   /* bmAttributes: self/bus powered */
    0xE1,   /* MaxPower 450 mA (225 x 2 mA) */
    /*Interface Descriptor*/
    0x09,   /* bLength: Interface Descriptor size */
    USB_INTERFACE_DESCRIPTOR_TYPE,  /* bDescriptorType: Interface */
    /* Interface descriptor type */
    0x00,   /* bInterfaceNumber: Number of Interface */
    0x00,   /* bAlternateSetting: Alternate setting */
    0x01,   /* bNumEndpoints: One endpoints used */
    0x02,   /* bInterfaceClass: Communication Interface Class */
    0x02,   /* bInterfaceSubClass: Abstract Control Model */
    0x01,   /* bInterfaceProtocol: Common AT commands */
    0x00,   /* iInterface: */
    /*Header Functional Descriptor*/
    0x05,   /* bLength: Endpoint Descriptor size */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x00,   /* bDescriptorSubtype: Header Func Desc */
    0x10,   /* bcdCDC: spec release number */
    0x01,
    /*Call Management Functional Descriptor*/
    0x05,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x01,   /* bDescriptorSubtype: Call Management Func Desc */
    0x00,   /* bmCapabilities: D0+D1 */
    0x01,   /* bDataInterface: 1 */
    /*ACM Functional Descriptor*/
    0x04,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
    0x02,   /* bmCapabilities */
    /*Union Functional Descriptor*/
    0x05,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x06,   /* bDescriptorSubtype: Union func desc */
    0x00,   /* bMasterInterface: Communication class interface */
    0x01,   /* bSlaveInterface0: Data Class Interface */
    /*Endpoint 2 Descriptor*/
    0x07,   /* bLength: Endpoint Descriptor size */
    USB_ENDPOINT_DESCRIPTOR_TYPE,   /* bDescriptorType: Endpoint */
    0x82,   /* bEndpointAddress: (IN2) */
    0x03,   /* bmAttributes: Interrupt */
    VCP_INT_SIZE,      /* wMaxPacketSize: */
    0x00,
    0xFF,   /* bInterval: */
    /*Data class interface descriptor*/
    0x09,   /* bLength: Endpoint Descriptor size */
    USB_INTERFACE_DESCRIPTOR_TYPE,  /* bDescriptorType: */
    0x01,   /* bInterfaceNumber: Number of Interface */
    0x00,   /* bAlternateSetting: Alternate setting */
    0x02,   /* bNumEndpoints: Two endpoints used */
    0x0A,   /* bInterfaceClass: CDC */
    0x00,   /* bInterfaceSubClass: */
    0x00,   /* bInterfaceProtocol: */
    0x00,   /* iInterface: */
    /*Endpoint 3 Descriptor*/
    0x07,   /* bLength: Endpoint Descriptor size */
    USB_ENDPOINT_DESCRIPTOR_TYPE,   /* bDescriptorType: Endpoint */
    0x03,   /* bEndpointAddress: (OUT3) */
    0x02,   /* bmAttributes: Bulk */
    VCP_DATA_SIZE,             /* wMaxPacketSize: */
    0x00,
    0x00,   /* bInterval: ignore for Bulk transfer */
    /*Endpoint 1 Descriptor*/
    0x07,   /* bLength: Endpoint Descriptor size */
    USB_ENDPOINT_DESCRIPTOR_TYPE,   /* bDescriptorType: Endpoint */
    0x81,   /* bEndpointAddress: (IN1) */
    0x02,   /* bmAttributes: Bulk */
    VCP_DATA_SIZE,             /* wMaxPacketSize: */
    0x00,
    0x00    /* bInterval */
  };
#endif

#ifdef USB_HID_ENABLE
/* USB Standard Device Descriptor */
const uint8_t HID_DeviceDescriptor[HID_SIZ_DEVICE_DESC] =
  {
    0x12,   /*bLength */
    USB_DEVICE_DESCRIPTOR_TYPE,   /*bDescriptorType*/
    0x00,   /*bcdUSB */
    0x02,
    0x00,   /*bDeviceClass*/
    0x00,   /*bDeviceSubClass*/
    0x00,   /*bDeviceProtocol*/
    0x40,   /*bMaxPacketSize 64*/
    0x50,
    0x1D,   /* idVendor = 0x1D50 */
    0x7D,
    0x60,   /* idProduct = 0x607D */
    0x00,
    0x02,   /* bcdDevice = 2.00 */
    1,      /*Index of string descriptor describing manufacturer */
    2,      /*Index of string descriptor describing product*/
    3,      /*Index of string descriptor describing the device serial number */
    0x01    /*bNumConfigurations*/
  };

/* USB Configuration Descriptor */
/*   All Descriptors (Configuration, Interface, Endpoint, Class, Vendor */
const uint8_t HID_ConfigDescriptor[HID_SIZ_CONFIG_DESC] =
  {
    0x09,   /* bLength: Configuration Descriptor size */
    USB_CONFIGURATION_DESCRIPTOR_TYPE, /* bDescriptorType: Configuration */
    HID_SIZ_CONFIG_DESC,  /* wTotalLength: Bytes returned */
    0x00,
    0x01,   /*bNumInterfaces: 1 interface*/
    0x01,   /*bConfigurationValue: Configuration value*/
    0x00,   /*iConfiguration: Index of string descriptor describing the configuration*/
    0xE0,   /*bmAttributes: Self powered */
    0x32,   /*MaxPower 100 mA: this current is used for detecting Vbus*/

    /************** Descriptor of HID Mouse interface ****************/
    /* 09 */
    0x09,   /*bLength: Interface Descriptor size*/
    USB_INTERFACE_DESCRIPTOR_TYPE,/*bDescriptorType: Interface descriptor type*/
    0x00,   /*bInterfaceNumber: Number of Interface*/
    0x00,   /*bAlternateSetting: Alternate setting*/
    0x01,   /*bNumEndpoints*/
    0x03,   /*bInterfaceClass: HID*/
    0x01,   /*bInterfaceSubClass : 1=BOOT, 0=no boot*/
    0x02,   /*nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse*/
    0,      /*iInterface: Index of string descriptor*/
    /******************** Descriptor of HID Mouse HID ********************/
    /* 18 */
    0x09,   /*bLength: HID Descriptor size*/
    HID_DESCRIPTOR_TYPE, /*bDescriptorType: HID*/
    0x00,   /*bcdHID: HID Class Spec release number*/
    0x01,
    0x00,   /*bCountryCode: Hardware target country*/
    0x01,   /*bNumDescriptors: Number of HID class descriptors to follow*/
    0x22,   /*bDescriptorType*/
    HID_SIZ_REPORT_DESC,/*wItemLength: Total length of Report descriptor*/
    0x00,
    /******************** Descriptor of HID Mouse endpoint ********************/
    /* 27 */
    0x07,   /*bLength: Endpoint Descriptor size*/
    USB_ENDPOINT_DESCRIPTOR_TYPE, /*bDescriptorType:*/

    0x81,   /*bEndpointAddress: Endpoint Address (IN)*/
    0x03,   /*bmAttributes: Interrupt endpoint*/
    0x04,   /*wMaxPacketSize: 4 Byte max */
    0x00,
    0x20,   /*bInterval: Polling Interval (32 ms)*/
    /* 34 */
  };

const uint8_t HID_ReportDescriptor[HID_SIZ_REPORT_DESC] =
  {
    0x05,   /*Usage Page(Generic Desktop)*/
    0x01,
    0x09,   /*Usage(Mouse)*/
    0x02,
    0xA1,   /*Collection(Logical)*/
    0x01,
    0x09,   /*Usage(Pointer)*/
    0x01,
    /* 8 */
    0xA1,   /*Collection(Linked)*/
    0x00,
    0x05,   /*Usage Page(Buttons)*/
    0x09,
    0x19,   /*Usage Minimum(1)*/
    0x01,
    0x29,   /*Usage Maximum(3)*/
    0x03,
    /* 16 */
    0x15,   /*Logical Minimum(0)*/
    0x00,
    0x25,   /*Logical Maximum(1)*/
    0x01,
    0x95,   /*Report Count(3)*/
    0x03,
    0x75,   /*Report Size(1)*/
    0x01,
    /* 24 */
    0x81,   /*Input(Variable)*/
    0x02,
    0x95,   /*Report Count(1)*/
    0x01,
    0x75,   /*Report Size(5)*/
    0x05,
    0x81,   /*Input(Constant,Array)*/
    0x01,
    /* 32 */
    0x05,   /*Usage Page(Generic Desktop)*/
    0x01,
    0x09,   /*Usage(X axis)*/
    0x30,
    0x09,   /*Usage(Y axis)*/
    0x31,
    0x09,   /*Usage(Wheel)*/
    0x38,
    /* 40 */
    0x15,   /*Logical Minimum(-127)*/
    0x81,
    0x25,   /*Logical Maximum(127)*/
    0x7F,
    0x75,   /*Report Size(8)*/
    0x08,
    0x95,   /*Report Count(3)*/
    0x03,
    /* 48 */
    0x81,   /*Input(Variable, Relative)*/
    0x06,
    0xC0,   /*End Collection*/
    0x09,
    0x3c,
    0x05,
    0xff,
    0x09,
    /* 56 */
    0x01,
    0x15,
    0x00,
    0x25,
    0x01,
    0x75,
    0x01,
    0x95,
    /* 64 */
    0x02,
    0xb1,
    0x22,
    0x75,
    0x06,
    0x95,
    0x01,
    0xb1,
    /* 72 */
    0x01,
    0xc0
  };
#endif

/* USB String Descriptors */
const uint8_t USB_StringLangID[USB_SIZ_STRING_LANGID] =
  {
    USB_SIZ_STRING_LANGID,
    USB_STRING_DESCRIPTOR_TYPE,
    0x09,
    0x04 /* LangID = 0x0409: U.S. English */
  };

const uint8_t USB_StringVendor[USB_SIZ_STRING_VENDOR] =
  {
    USB_SIZ_STRING_VENDOR,     /* Size of Vendor string */
    USB_STRING_DESCRIPTOR_TYPE,             /* bDescriptorType*/
    /* Manufacturer: "Spark Devices     " */
    'S', 0, 'p', 0, 'a', 0, 'r', 0, 'k', 0, ' ', 0, 'D', 0, 'e', 0,
    'v', 0, 'i', 0, 'c', 0, 'e', 0, 's', 0, ' ', 0, ' ', 0, ' ', 0,
    ' ', 0, ' ', 0
  };

const uint8_t USB_StringProduct[USB_SIZ_STRING_PRODUCT] =
  {
    USB_SIZ_STRING_PRODUCT,          /* bLength */
    USB_STRING_DESCRIPTOR_TYPE,        /* bDescriptorType */
    /* Product name: "Spark Core with WiFi  " */
    'S', 0, 'p', 0, 'a', 0, 'r', 0, 'k', 0, ' ', 0, 'C', 0, 'o', 0,
    'r', 0, 'e', 0, ' ', 0, 'w', 0, 'i', 0, 't', 0, 'h', 0, ' ', 0,
    'W', 0, 'i', 0, 'F', 0, 'i', 0, ' ', 0, ' ', 0, ' ', 0, ' ', 0
  };

uint8_t USB_StringSerial[USB_SIZ_STRING_SERIAL] =
  {
    USB_SIZ_STRING_SERIAL,           /* bLength */
    USB_STRING_DESCRIPTOR_TYPE,                   /* bDescriptorType */
    'C', 0, 'O', 0, 'R', 0, 'E', 0, ' ', 0
  };

