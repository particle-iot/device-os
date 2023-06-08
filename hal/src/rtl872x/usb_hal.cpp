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

#undef LOG_COMPILE_TIME_LEVEL
#include "usb_hal.h"
#include "usbd_device.h"
#include "usbd_control.h"
#include "usbd_driver.h"
#include "usbd_cdc.h"
#include <mutex>
#include "usb_settings.h"
#include "usbd_hid.h"

using namespace particle::usbd;

CdcClassDriver& getCdcClassDriver() {
    static CdcClassDriver cdc;
    return cdc;
}

namespace {

// Avoid global object construction order issues
static Device& getUsbDevice() {
    static Device dev;
    return dev;
}

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
    LOBYTE(USBD_VID_PARTICLE),     /*idVendor*/
    HIBYTE(USBD_VID_PARTICLE),     /*idVendor*/
    LOBYTE(USBD_PID_CDC),       /*idProduct*/
    HIBYTE(USBD_PID_CDC),       /*idProduct*/
    LOBYTE(0x0251),             /*bcdDevice (2.51) */
    HIBYTE(0x0251),             /*bcdDevice (2.51) */
    STRING_IDX_MANUFACTURER,    /*Index of manufacturer  string*/
    STRING_IDX_PRODUCT,         /*Index of product string*/
    STRING_IDX_SERIAL,          /*Index of serial number string*/
    0x01                        /*bNumConfigurations*/
};

} // anonymous

void HAL_USB_Init(void) {
    static std::once_flag onceFlag;
    std::call_once(onceFlag, []() {
        getUsbDevice().setDeviceDescriptor(sDeviceDescriptor, sizeof(sDeviceDescriptor));
        getUsbDevice().registerDriver(RtlUsbDriver::instance());

        getUsbDevice().registerClass(ControlInterfaceClassDriver::instance());
        getUsbDevice().registerClass(&getCdcClassDriver());
        getUsbDevice().registerClass(HidClassDriver::instance());

        ControlInterfaceClassDriver::instance()->enable(true);
        // Only enabled if HAL_USB_USART_Init() was called to configure buffers
        // getCdcClassDriver().enable(true);
        HAL_USB_Attach();
    });
}

void HAL_USB_Attach() {
    getUsbDevice().attach();
}

void HAL_USB_Detach() {
    getUsbDevice().detach();
}

void HAL_USB_Set_Vendor_Request_Callback(HAL_USB_Vendor_Request_Callback cb, void* p) {
    ControlInterfaceClassDriver::instance()->setVendorRequestCallback(cb, p);
}

void HAL_USB_Set_Vendor_Request_State_Callback(HAL_USB_Vendor_Request_State_Callback cb, void* p) {
    ControlInterfaceClassDriver::instance()->setVendorRequestStateCallback(cb, p);
}

void HAL_USB_USART_Init(HAL_USB_USART_Serial serial, const HAL_USB_USART_Config* config) {
    if (serial != HAL_USB_USART_SERIAL) {
        return;
    }
    if (getCdcClassDriver().isEnabled()) {
        return;
    }
    // FIXME: figure out what's going on here
    if (!config ||
            (config->rx_buffer == nullptr ||
             config->rx_buffer_size == 0 ||
             config->tx_buffer == nullptr ||
             config->tx_buffer_size == 0)) {
        uint8_t* txBuffer = (uint8_t*)malloc(USB_TX_BUFFER_SIZE);
        uint8_t* rxBuffer = (uint8_t*)malloc(USB_RX_BUFFER_SIZE);
        if (txBuffer && rxBuffer) {
            getCdcClassDriver().initBuffers(rxBuffer, USB_RX_BUFFER_SIZE, txBuffer, USB_TX_BUFFER_SIZE);
        } else {
            if (txBuffer) {
                free(txBuffer);
            }
            if (rxBuffer) {
                free(rxBuffer);
            }
        }
    } else {
        getCdcClassDriver().initBuffers(config->rx_buffer, config->rx_buffer_size, config->tx_buffer, config->tx_buffer_size);
    }
}

void HAL_USB_USART_Begin(HAL_USB_USART_Serial serial, uint32_t baud, void *reserved) {
    if (serial != HAL_USB_USART_SERIAL) {
        return;
    }
    if (!getCdcClassDriver().isEnabled() && getCdcClassDriver().buffersConfigured()) {
        HAL_USB_Detach();
        getCdcClassDriver().enable(true);
        HAL_USB_Init();
        HAL_USB_Attach();
    }
}

void HAL_USB_USART_End(HAL_USB_USART_Serial serial) {
    if (serial != HAL_USB_USART_SERIAL) {
        return;
    }
    if (getCdcClassDriver().isEnabled()) {
        HAL_USB_Detach();
        getCdcClassDriver().enable(false);
        HAL_USB_Attach();
    }
}

unsigned int HAL_USB_USART_Baud_Rate(HAL_USB_USART_Serial serial) {
    if (serial != HAL_USB_USART_SERIAL) {
        return 0;
    }
    cdc::LineCoding lineCoding = {};
    if (getCdcClassDriver().getLineCoding(lineCoding)) {
        return 0;
    }
    return lineCoding.dwDTERate;
}

int32_t HAL_USB_USART_Available_Data(HAL_USB_USART_Serial serial) {
    if (serial != HAL_USB_USART_SERIAL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    return getCdcClassDriver().available();
}

int32_t HAL_USB_USART_Available_Data_For_Write(HAL_USB_USART_Serial serial) {
    if (serial != HAL_USB_USART_SERIAL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (!HAL_USB_USART_Is_Connected(serial)) {
        return -1;
    }
    return getCdcClassDriver().availableForWrite();
}

int32_t HAL_USB_USART_Receive_Data(HAL_USB_USART_Serial serial, uint8_t peek) {
    if (serial != HAL_USB_USART_SERIAL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    uint8_t c;
    int r = 0;
    if (!peek) {
        r = getCdcClassDriver().read(&c, sizeof(c));
    } else {
        r = getCdcClassDriver().peek(&c, sizeof(c));
    }
    if (r == sizeof(c)) {
        return c;
    }
    return r;
}

int32_t HAL_USB_USART_Send_Data(HAL_USB_USART_Serial serial, uint8_t data) {
    if (serial != HAL_USB_USART_SERIAL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    // Just in case for now
    if ((__get_PRIMASK() & 1)) {
        return -1;
    }
    int32_t available = -1;
    do {
        available = HAL_USB_USART_Available_Data_For_Write(serial);
        if (available > 0) {
            break;
        }
        // TODO: check thread priorities here and delay only conditionally?
        HAL_Delay_Milliseconds(1);
    } while (available < 1 && available != -1);
    if (HAL_USB_USART_Is_Connected(serial) && available > 0) {
        return getCdcClassDriver().write(&data, sizeof(data));
    }
    return -1;
}

void HAL_USB_USART_Flush_Data(HAL_USB_USART_Serial serial) {
    if (serial != HAL_USB_USART_SERIAL) {
        return;
    }
    return getCdcClassDriver().flush();
}

bool HAL_USB_USART_Is_Enabled(HAL_USB_USART_Serial serial) {
    if (serial != HAL_USB_USART_SERIAL) {
        return false;
    }
    return getCdcClassDriver().isEnabled();
}

bool HAL_USB_USART_Is_Connected(HAL_USB_USART_Serial serial) {
    if (serial != HAL_USB_USART_SERIAL) {
        return false;
    }
    return getCdcClassDriver().isConnected();
}

void USB_USART_LineCoding_BitRate_Handler(void (*handler)(uint32_t bitRate)) {
    // Old USB API, just for compatibility in main.cpp
    // Enable Serial by default
    HAL_USB_USART_LineCoding_BitRate_Handler(handler, NULL);
}

int32_t HAL_USB_USART_LineCoding_BitRate_Handler(void (*handler)(uint32_t bitRate), void* reserved) {
    // Enable Serial by default
    HAL_USB_USART_Init(HAL_USB_USART_SERIAL, nullptr);
    HAL_USB_USART_Begin(HAL_USB_USART_SERIAL, 9600, nullptr);
    getCdcClassDriver().onSetLineCoding([handler](cdc::LineCoding lineCoding) -> void {
        handler(lineCoding.dwDTERate);
    });
    return 0;
}

HAL_USB_State HAL_USB_Get_State(void* reserved) {
    return (HAL_USB_State)0;
}

int HAL_USB_Set_State_Change_Callback(HAL_USB_State_Callback cb, void* context, void* reserved) {
    return 0;
}
