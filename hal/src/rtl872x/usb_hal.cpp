/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "usb_hal.h"
#include "usbd_device.h"
#include "usbd_control.h"
#include "usbd_driver.h"
#include <mutex>

using namespace particle::usbd;

namespace {

Device gUsbDevice;

#define LOBYTE(x)  ((uint8_t)(x & 0x00FF))
#define HIBYTE(x)  ((uint8_t)((x & 0xFF00) >>8))

const uint8_t sDeviceDescriptor[] = {
    0x12,                       /*bLength */
    DESCRIPTOR_DEVICE,          /*bDescriptorType*/
    0x00,                       /*bcdUSB */
    0x02,
    0xef,                       /*bDeviceClass: Misc */
    0x02,                       /*bDeviceSubClass*/
    0x01,                       /*bDeviceProtocol*/
    EP0_MAX_PACKET_SIZE,        /*bMaxPacketSize*/
    LOBYTE(USBD_VID_SPARK),     /*idVendor*/
    HIBYTE(USBD_VID_SPARK),     /*idVendor*/
    LOBYTE(USBD_PID_CDC),       /*idProduct*/
    HIBYTE(USBD_PID_CDC),       /*idProduct*/
    LOBYTE(0x0250),             /*bcdDevice (2.50) */
    HIBYTE(0x0250),             /*bcdDevice (2.50) */
    STRING_IDX_MANUFACTURER,    /*Index of manufacturer  string*/
    STRING_IDX_PRODUCT,         /*Index of product string*/
    STRING_IDX_SERIAL,          /*Index of serial number string*/
    0x01                        /*bNumConfigurations*/
};

} // anonymous

void HAL_USB_Init(void) {
    static std::once_flag onceFlag;
    std::call_once(onceFlag, []() {
        gUsbDevice.setDeviceDescriptor(sDeviceDescriptor, sizeof(sDeviceDescriptor));
        gUsbDevice.registerDriver(RtlUsbDriver::instance());
        // gUsbDevice.registerClass(CdcClassDriver::instance());
        gUsbDevice.registerClass(ControlInterfaceClassDriver::instance());
        ControlInterfaceClassDriver::instance()->enable(true);
        HAL_USB_Attach();
    });
}

void HAL_USB_Attach() {
    gUsbDevice.attach();
}

void HAL_USB_Detach() {
    gUsbDevice.detach();
}

void HAL_USB_Set_Vendor_Request_Callback(HAL_USB_Vendor_Request_Callback cb, void* p) {
    ControlInterfaceClassDriver::instance()->setVendorRequestCallback(cb, p);
}

void HAL_USB_Set_Vendor_Request_State_Callback(HAL_USB_Vendor_Request_State_Callback cb, void* p) {
    ControlInterfaceClassDriver::instance()->setVendorRequestStateCallback(cb, p);
}


void HAL_USB_USART_Init(HAL_USB_USART_Serial serial, const HAL_USB_USART_Config* config) {
    return;
}

void HAL_USB_USART_Begin(HAL_USB_USART_Serial serial, uint32_t baud, void *reserved) {
    return;
}

void HAL_USB_USART_End(HAL_USB_USART_Serial serial) {
    return;
}

unsigned int HAL_USB_USART_Baud_Rate(HAL_USB_USART_Serial serial) {
    return 0;
}

int32_t HAL_USB_USART_Available_Data(HAL_USB_USART_Serial serial) {
    return -1;
}

int32_t HAL_USB_USART_Available_Data_For_Write(HAL_USB_USART_Serial serial) {
    return -1;
}

int32_t HAL_USB_USART_Receive_Data(HAL_USB_USART_Serial serial, uint8_t peek) {
    return -1;
}

int32_t HAL_USB_USART_Send_Data(HAL_USB_USART_Serial serial, uint8_t data) {
    return -1;
}

void HAL_USB_USART_Flush_Data(HAL_USB_USART_Serial serial) {
    return;
}

bool HAL_USB_USART_Is_Enabled(HAL_USB_USART_Serial serial) {
    return false;
}

bool HAL_USB_USART_Is_Connected(HAL_USB_USART_Serial serial) {
    return false;
}

void USB_USART_LineCoding_BitRate_Handler(void (*handler)(uint32_t bitRate)) {
    return;
}

int32_t HAL_USB_USART_LineCoding_BitRate_Handler(void (*handler)(uint32_t bitRate), void* reserved) {
    return 0;
}

int32_t USB_USART_Flush_Output(unsigned timeout, void* reserved) {
    return 0;
}

HAL_USB_State HAL_USB_Get_State(void* reserved) {
    return (HAL_USB_State)0;
}

int HAL_USB_Set_State_Change_Callback(HAL_USB_State_Callback cb, void* context, void* reserved) {
    return 0;
}
