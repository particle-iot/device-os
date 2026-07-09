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
#include <algorithm>
#include <mutex>
#include "usb_settings.h"
#include "usbd_hid.h"
// FIXME: not ideal, directly using freertos task APIs
#include "FreeRTOS.h"
#include "task.h"
#include "call_once.h"

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
    static particle::OnceFlag onceFlag;
    particle::CallOnce(onceFlag, []() {
        getUsbDevice().setDeviceDescriptor(sDeviceDescriptor, sizeof(sDeviceDescriptor));
        getUsbDevice().registerDriver(RtlUsbDriver::instance());

        getUsbDevice().registerClass(ControlInterfaceClassDriver::instance());
        getUsbDevice().registerClass(&getCdcClassDriver());
        getUsbDevice().registerClass(HidClassDriver::instance());

        ControlInterfaceClassDriver::instance()->enable(true);
        // Only enabled if HAL_USB_USART_Init() was called to configure buffers
#if defined (START_DFU_FLASHER_SERIAL_SPEED)
        HAL_USB_USART_Init(HAL_USB_USART_SERIAL, nullptr);
        getCdcClassDriver().enable(true);
#endif
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
    const bool haveConfig = (config != nullptr &&
            config->rx_buffer != nullptr && config->rx_buffer_size != 0 &&
            config->tx_buffer != nullptr && config->tx_buffer_size != 0);

    const bool reconfigure = getCdcClassDriver().isEnabled();
    if (reconfigure) {
        if (!haveConfig) {
            return;
        }
        HAL_USB_Detach();
        getCdcClassDriver().enable(false);
    }

    if (!haveConfig) {
        // Reused across calls to avoid leaking on repeated default init (matches Gen 3).
        static uint8_t* sTxBuffer = nullptr;
        static uint8_t* sRxBuffer = nullptr;
        if (!sTxBuffer) {
            sTxBuffer = (uint8_t*)malloc(USB_TX_BUFFER_SIZE);
        }
        if (!sRxBuffer) {
            sRxBuffer = (uint8_t*)malloc(USB_RX_BUFFER_SIZE);
        }
        if (sTxBuffer && sRxBuffer) {
            getCdcClassDriver().initBuffers(sRxBuffer, USB_RX_BUFFER_SIZE, sTxBuffer, USB_TX_BUFFER_SIZE);
        }
    } else {
        getCdcClassDriver().initBuffers(config->rx_buffer, config->rx_buffer_size, config->tx_buffer, config->tx_buffer_size);
    }

    if (reconfigure) {
        getCdcClassDriver().enable(true);
        HAL_USB_Init();
        HAL_USB_Attach();
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

int32_t HAL_USB_USART_Available_Data_protected(HAL_USB_USART_Serial serial) {
    CHECK_SECURITY_MODE_PROTECTED();
    return HAL_USB_USART_Available_Data(serial);
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

int32_t HAL_USB_USART_Available_Data_For_Write_protected(HAL_USB_USART_Serial serial) {
    CHECK_SECURITY_MODE_PROTECTED();
    return HAL_USB_USART_Available_Data_For_Write(serial);
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

int32_t HAL_USB_USART_Receive_Data_protected(HAL_USB_USART_Serial serial, uint8_t peek) {
    CHECK_SECURITY_MODE_PROTECTED();
    return HAL_USB_USART_Receive_Data(serial, peek);
}

static int32_t HAL_USB_USART_Send_Data_Multiple(HAL_USB_USART_Serial serial, const uint8_t* data, size_t size) {
    if (serial != HAL_USB_USART_SERIAL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    // Just in case for now
    if ((__get_PRIMASK() & 1) || (__get_BASEPRI() != 0)) {
        return -1;
    }
    int32_t available = HAL_USB_USART_Available_Data_For_Write(serial);
    if (available == 0) {
        bool needToYield = false;
        auto prio = uxTaskPriorityGet(nullptr);
        if (prio >= rtl::RTL_USBD_ISR_PROCESSING_THREAD_PRIORITY) {
            needToYield = true;
        }
        while (true) {
            available = HAL_USB_USART_Available_Data_For_Write(serial);
            if (available > 0 || available < 0) {
                break;
            }
            if (needToYield) {
                HAL_Delay_Milliseconds(1);
            }
        }
    }
    if (available > 0 && HAL_USB_USART_Is_Connected(serial)) {
        return getCdcClassDriver().write(data, size);
    }
    return -1;
}

int32_t HAL_USB_USART_Send_Data(HAL_USB_USART_Serial serial, uint8_t data) {
    return HAL_USB_USART_Send_Data_Multiple(serial, &data, 1);
}

int32_t HAL_USB_USART_Send_Data_protected(HAL_USB_USART_Serial serial, uint8_t data) {
    CHECK_SECURITY_MODE_PROTECTED();
    return HAL_USB_USART_Send_Data(serial, data);
}

void HAL_USB_USART_Flush_Data(HAL_USB_USART_Serial serial) {
    if (serial != HAL_USB_USART_SERIAL) {
        return;
    }
    return getCdcClassDriver().flush();
}

void HAL_USB_USART_Flush_Data_protected(HAL_USB_USART_Serial serial) {
    if (security_mode_get(NULL) == MODULE_INFO_SECURITY_MODE_PROTECTED) {
        return;
    }
    return HAL_USB_USART_Flush_Data(serial);
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

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int hal_usb_cdc_pvt_get_event_group_handle(EventGroupHandle_t* handle) {
    return getCdcClassDriver().eventGroup(handle);
}

int hal_usb_cdc_pvt_wait_event(uint32_t events, system_tick_t timeout) {
    return HAL_USB_USART_Wait_Event(HAL_USB_USART_SERIAL, events, timeout, nullptr);
}

int hal_usb_cdc_pvt_send_data(const char* data, size_t size) {
    return HAL_USB_USART_Send_Buffer(HAL_USB_USART_SERIAL, data, size);
}

int hal_usb_cdc_pvt_recv_data(char* data, size_t size) {
    return HAL_USB_USART_Receive_Buffer(HAL_USB_USART_SERIAL, data, size);
}

int32_t HAL_USB_USART_Send_Buffer(HAL_USB_USART_Serial serial, const void* data, size_t size) {
    if (serial != HAL_USB_USART_SERIAL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (size == 0) {
        return 0;
    }
    if ((__get_PRIMASK() & 1) || (__get_BASEPRI() != 0)) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!HAL_USB_USART_Is_Connected(serial)) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    int32_t available = HAL_USB_USART_Available_Data_For_Write(serial);
    if (available < 0) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (available == 0) {
        return 0;
    }
    size_t writeSize = std::min((size_t)available, size);
    return getCdcClassDriver().write((const uint8_t*)data, writeSize);
}

int32_t HAL_USB_USART_Receive_Buffer(HAL_USB_USART_Serial serial, void* data, size_t size) {
    if (serial != HAL_USB_USART_SERIAL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (size == 0) {
        return 0;
    }
    return getCdcClassDriver().read((uint8_t*)data, size);
}

int32_t HAL_USB_USART_Peek_Buffer(HAL_USB_USART_Serial serial, void* data, size_t size) {
    if (serial != HAL_USB_USART_SERIAL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (size == 0) {
        return 0;
    }
    return getCdcClassDriver().peek((uint8_t*)data, size);
}

int HAL_USB_USART_Wait_Event(HAL_USB_USART_Serial serial, uint32_t events, system_tick_t timeout, void* reserved) {
    if (serial != HAL_USB_USART_SERIAL) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    (void)reserved;
    if (!events) {
        return 0;
    }
    return getCdcClassDriver().waitEvent(events, timeout);
}

int32_t HAL_USB_USART_Send_Buffer_protected(HAL_USB_USART_Serial serial, const void* data, size_t size) {
    CHECK_SECURITY_MODE_PROTECTED();
    return HAL_USB_USART_Send_Buffer(serial, data, size);
}

int32_t HAL_USB_USART_Receive_Buffer_protected(HAL_USB_USART_Serial serial, void* data, size_t size) {
    CHECK_SECURITY_MODE_PROTECTED();
    return HAL_USB_USART_Receive_Buffer(serial, data, size);
}

int32_t HAL_USB_USART_Peek_Buffer_protected(HAL_USB_USART_Serial serial, void* data, size_t size) {
    CHECK_SECURITY_MODE_PROTECTED();
    return HAL_USB_USART_Peek_Buffer(serial, data, size);
}

int HAL_USB_USART_Wait_Event_protected(HAL_USB_USART_Serial serial, uint32_t events, system_tick_t timeout, void* reserved) {
    CHECK_SECURITY_MODE_PROTECTED();
    return HAL_USB_USART_Wait_Event(serial, events, timeout, reserved);
}
#ifdef __cplusplus
}
#endif // __cplusplus
