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

#include "usbd_driver.h"
#include "system_error.h"
#include "service_debug.h"
#include <mutex>

using namespace particle::usbd;

#define CHECK_RTL_USB(_expr) \
        ({ \
            const auto _ret = _expr; \
            if (_ret < 0) { \
                return HAL_ERR_HW; \
            } \
            _ret; \
        })

#define CHECK_RTL_USB_TO_SYSTEM(_expr) \
        ({ \
            const auto _ret = _expr; \
            if (_ret != HAL_OK) { \
                return SYSTEM_ERROR_UNKNOWN; \
            } \
            _ret; \
        })


namespace {

SetupRequest sLastUsbSetupRequest = {};

int rtlDescriptorTypeToDriver(usbd_desc_type_t desc) {
    switch (desc) {
    case USBD_DESC_DEVICE: {
        return DESCRIPTOR_DEVICE;
    }
    case USBD_DESC_CONFIG: {
        return DESCRIPTOR_CONFIGURATION;
    }
    case USBD_DESC_OTHER_SPEED_CONFIG: {
        return DESCRIPTOR_OTHER_SPEED_CONFIGURATION;
    }
    case USBD_DESC_DEVICE_QUALIFIER: {
        return DESCRIPTOR_DEVICE_QUALIFIER;
    }
    case USBD_DESC_LANGID_STR:
    case USBD_DESC_MFG_STR:
    case USBD_DESC_PRODUCT_STR:
    case USBD_DESC_SN_STR:
    case USBD_DESC_CONFIG_STR:
    case USBD_DESC_INTERFACE_STR: {
        return DESCRIPTOR_STRING;
    }
    }
    return SYSTEM_ERROR_UNKNOWN;
}

Speed rtlSpeedToDriver(usbd_speed_type_t speed) {
    switch (speed) {
    case USBD_SPEED_HIGH: {
        return Speed::HIGH;
    }
    case USBD_SPEED_LOW: {
        return Speed::LOW;
    }
    case USBD_SPEED_FULL:
    default: {
        return Speed::FULL;
    }
    }
}

uint8_t endpointTypeToRtl(EndpointType type) {
    switch (type) {
    case EndpointType::CONTROL: {
        return USBD_EP_TYPE_CTRL;
    }
    case EndpointType::INTERRUPT: {
        return USBD_EP_TYPE_INTR;
    }
    case EndpointType::ISOCHRONOUS: {
        return USBD_EP_TYPE_ISOC;
    }
    case EndpointType::BULK: {
        return USBD_EP_TYPE_BULK;
    }
    }
    return 0;
}

} // anonymous

// FIXME: This is a nasty workaround for the SDK USB driver
// where it fails to deliver requests for strings higher than STRING_IDX_SERIAL
extern "C" {

int usbd_hal_read_packet(void* pcd, void* ptr, uint32_t size);
int __real_usbd_hal_read_packet(void* pcd, void* ptr, uint32_t size);

int __wrap_usbd_hal_read_packet(void* pcd, void* ptr, uint32_t size) {
    int r = __real_usbd_hal_read_packet(pcd, ptr, size);
    if (size == sizeof(sLastUsbSetupRequest)) {
        memcpy(&sLastUsbSetupRequest, ptr, sizeof(sLastUsbSetupRequest));
        auto req = static_cast<SetupRequest*>(ptr);
        if (req->bmRequestType == (SetupRequest::DIRECTION_DEVICE_TO_HOST | SetupRequest::RECIPIENT_DEVICE | SetupRequest::TYPE_STANDARD)
                && req->bRequest == SetupRequest::REQUEST_GET_DESCRIPTOR && (req->wValue & 0xff) > STRING_IDX_SERIAL) {
            req->wValue &= 0xff00;
            RtlUsbDriver::instance()->halReadPacketFixup(ptr);
        }
    }
    return r;
}

} // extern "C"

RtlUsbDriver* RtlUsbDriver::instance() {
    static RtlUsbDriver driver;
    return &driver;
}

RtlUsbDriver::RtlUsbDriver() {
}

RtlUsbDriver::~RtlUsbDriver() {
}

int RtlUsbDriver::attach() {
    // NOTE: these calls may fail
    usbd_init(&usbdCfg_);
    usbd_register_class(&classDrv_);
    return 0;
}

int RtlUsbDriver::detach() {
    usbd_unregister_class();
    usbd_deinit();
    return 0;
}

int RtlUsbDriver::suspend() {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int RtlUsbDriver::resume() {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

DeviceState RtlUsbDriver::getState() const {
    if (rtlDev_) {
        switch (rtlDev_->dev_state) {
        case USBD_STATE_INIT: {
            return DeviceState::DETACHED;
        }
        case USBD_STATE_DEFAULT: {
            return DeviceState::DEFAULT;
        }
        case USBD_STATE_ADDRESSED: {
            return DeviceState::ADDRESSED;
        }
        case USBD_STATE_CONFIGURED: {
            return DeviceState::CONFIGURED;
        }
        case USBD_STATE_SUSPENDED: {
            return DeviceState::SUSPENDED;
        }
        }
    }

    return DeviceState::NONE;
}

int RtlUsbDriver::openEndpoint(unsigned ep, EndpointType type, size_t pakSize) {
    SPARK_ASSERT(rtlDev_);
    return CHECK_RTL_USB_TO_SYSTEM(usbd_ep_init(rtlDev_, ep, endpointTypeToRtl(type), pakSize));
}

int RtlUsbDriver::closeEndpoint(unsigned ep) {
    SPARK_ASSERT(rtlDev_);
    return CHECK_RTL_USB_TO_SYSTEM(usbd_ep_deinit(rtlDev_, ep));
}

int RtlUsbDriver::flushEndpoint(unsigned ep) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int RtlUsbDriver::stallEndpoint(unsigned ep) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int RtlUsbDriver::clearStallEndpoint(unsigned ep) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

EndpointStatus RtlUsbDriver::getEndpointStatus(unsigned ep) {
    // FIXME: there is an undocumented field in usbd_ep_t
    return EndpointStatus::NONE;
}

int RtlUsbDriver::setEndpointStatus(unsigned ep, EndpointStatus status) {
    // FIXME: there is an undocumented field in usbd_ep_t
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int RtlUsbDriver::transferIn(unsigned ep, const uint8_t* ptr, size_t size) {
    SPARK_ASSERT(rtlDev_);
    return CHECK_RTL_USB_TO_SYSTEM(usbd_ep_transmit(rtlDev_, ep | SetupRequest::DIRECTION_DEVICE_TO_HOST, (uint8_t*)ptr, size));
}

int RtlUsbDriver::transferOut(unsigned ep, uint8_t* ptr, size_t size) {
    SPARK_ASSERT(rtlDev_);
    return CHECK_RTL_USB_TO_SYSTEM(usbd_ep_receive(rtlDev_, ep & ~(SetupRequest::DIRECTION_DEVICE_TO_HOST), ptr, size));
}

int RtlUsbDriver::setupReply(SetupRequest* r, const uint8_t* data, size_t size) {
    SPARK_ASSERT(rtlDev_);
    if (data && size) {
        return CHECK_RTL_USB_TO_SYSTEM(usbd_ep0_transmit(rtlDev_, (uint8_t*)data, size));
    }
    return 0;
}


int RtlUsbDriver::setupReceive(SetupRequest* r, uint8_t* data, size_t size) {
    SPARK_ASSERT(rtlDev_);
    return CHECK_RTL_USB_TO_SYSTEM(usbd_ep0_receive(rtlDev_, data, size));
}

int RtlUsbDriver::setupError(SetupRequest* r) {
    setupError_ = true;
    return 0;
}

unsigned RtlUsbDriver::getEndpointMask() const {
    unsigned mask = 0;
    for (size_t i = 0; i < USBD_MAX_ENDPOINTS; i++) {
        mask |= (1 << i);
        mask |= (1 << i) << ENDPOINT_MASK_OUT_BITPOS;
    }
    return mask;
}

unsigned RtlUsbDriver::updateEndpointMask(unsigned mask) const {
    for (size_t i = 0; i < USBD_MAX_ENDPOINTS; i++) {
        if (mask & ((1 << i) | (1 << i) << ENDPOINT_MASK_OUT_BITPOS)) {
            // It's not possible to use endpoint 0x01 and 0x81, they are one and the same
            mask |= (1 << i);
            mask |= (1 << i) << ENDPOINT_MASK_OUT_BITPOS;
        }
    }
    return mask;
}

uint8_t* RtlUsbDriver::getDescriptorCb(usbd_desc_type_t desc, usbd_speed_type_t speed, uint16_t* len) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    auto type = rtlDescriptorTypeToDriver(desc);
    if (type < 0) {
        return nullptr;
    }
    auto s = rtlSpeedToDriver(speed);
    int r = SYSTEM_ERROR_UNKNOWN;
    if (type != DESCRIPTOR_STRING) {
        r = self->getDescriptor((DescriptorType)type, self->tempBuffer_, rtl::TEMP_BUFFER_SIZE, s, sLastUsbSetupRequest.wValue & 0xff);
    } else {
        r = self->getString(sLastUsbSetupRequest.wValue & 0xff, sLastUsbSetupRequest.wIndex, self->tempBuffer_, rtl::TEMP_BUFFER_SIZE);
    }
    if (r < 0) {
        *len = 0;
        return nullptr;
    }
    *len = (uint16_t)r;
    return self->tempBuffer_;
}

uint8_t RtlUsbDriver::setConfigCb(usb_dev_t* dev, uint8_t config) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    self->setDevReference(dev);
    return CHECK_RTL_USB(self->setConfig(config));
}

uint8_t RtlUsbDriver::clearConfigCb(usb_dev_t* dev, uint8_t config) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    self->setDevReference(dev);
    return CHECK_RTL_USB(self->clearConfig(config));
}

uint8_t RtlUsbDriver::setupCb(usb_dev_t* dev, usb_setup_req_t* req) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    self->setDevReference(dev);
    static_assert(sizeof(SetupRequest) == sizeof(usb_setup_req_t), "SetupRequest and usb_setup_req_t are expected to be of the same size");
    CHECK_RTL_USB(self->setupRequest(&sLastUsbSetupRequest));
    if (self->setupError_) {
        self->setupError_ = false;
        return HAL_ERR_HW;
    }
    return 0;
}

uint8_t RtlUsbDriver::sofCb(usb_dev_t* dev) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    self->setDevReference(dev);
    return CHECK_RTL_USB(self->startOfFrame());
}

uint8_t RtlUsbDriver::suspendCb(usb_dev_t* dev) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    self->setDevReference(dev);
    return CHECK_RTL_USB(self->notifySuspend());
}

uint8_t RtlUsbDriver::resumeCb(usb_dev_t* dev) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    self->setDevReference(dev);
    return CHECK_RTL_USB(self->notifyResume());
}

uint8_t RtlUsbDriver::ep0DataInCb(usb_dev_t* dev) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    self->setDevReference(dev);
    return CHECK_RTL_USB(self->notifyEpTransferDone(SetupRequest::DIRECTION_DEVICE_TO_HOST | 0x00, EndpointEvent::DONE, dev->ep0_data_len));
}

uint8_t RtlUsbDriver::ep0DataOutCb(usb_dev_t* dev) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    self->setDevReference(dev);
    self->fixupReceivedData();
    return CHECK_RTL_USB(self->notifyEpTransferDone(SetupRequest::DIRECTION_HOST_TO_DEVICE | 0x00, EndpointEvent::DONE, dev->ep0_data_len));
}

uint8_t RtlUsbDriver::epDataInCb(usb_dev_t* dev, uint8_t ep_num) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    self->setDevReference(dev);
    // NOTE: no way to tell the size of the last transfer
    CHECK_RTL_USB(self->notifyEpTransferDone(SetupRequest::DIRECTION_DEVICE_TO_HOST | ep_num, EndpointEvent::DONE, 0 /* dev->ep_in[ep_num & 0x7f].data_len */));
    return 0;
}

uint8_t RtlUsbDriver::epDataOutCb(usb_dev_t* dev, uint8_t ep_num, uint16_t len) {
    auto self = instance();
    std::lock_guard<RtlUsbDriver> lk(*self);

    self->setDevReference(dev);
    self->fixupReceivedData();
    CHECK_RTL_USB(self->notifyEpTransferDone(SetupRequest::DIRECTION_HOST_TO_DEVICE | ep_num, EndpointEvent::DONE, len));
    return 0;
}

void RtlUsbDriver::setDevReference(usb_dev_t* dev) {
    if (!rtlDev_) {
        rtlDev_ = dev;
    }
}

void RtlUsbDriver::halReadPacketFixup(void* ptr) {
    fixupPtr_ = ptr;
}

void RtlUsbDriver::fixupReceivedData() {
    if (fixupPtr_) {
        auto req = static_cast<SetupRequest*>(fixupPtr_);
        req->wValue = sLastUsbSetupRequest.wValue;
        fixupPtr_ = nullptr;
    }
}

bool RtlUsbDriver::lock() {
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    mutex_.lock();
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    return true;
}

void RtlUsbDriver::unlock() {
#if MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    mutex_.unlock();
#endif // MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
}
