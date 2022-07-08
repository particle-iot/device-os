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

#include "usbd_control.h"
#include <cstring>
#include <algorithm>
#include "usbd_wcid.h"
#include "appender.h"
#include "check.h"

using namespace particle::usbd;

namespace {

/* Extended Properties OS Descriptor */
static const uint8_t MSFT_EXTENDED_PROPERTIES_OS_DESCRIPTOR[] = {
    USB_WCID_EXT_PROP_OS_DESCRIPTOR(
        USB_WCID_DATA(
            /* bPropertyData "{20b6cfa4-6dc7-468a-a8db-faa7c23ddea5}" */
            '{', 0x00, '2', 0x00, '0', 0x00, 'b', 0x00,
            '6', 0x00, 'c', 0x00, 'f', 0x00, 'a', 0x00,
            '4', 0x00, '-', 0x00, '6', 0x00, 'd', 0x00,
            'c', 0x00, '7', 0x00, '-', 0x00, '4', 0x00,
            '6', 0x00, '8', 0x00, 'a', 0x00, '-', 0x00,
            'a', 0x00, '8', 0x00, 'd', 0x00, 'b', 0x00,
            '-', 0x00, 'f', 0x00, 'a', 0x00, 'a', 0x00,
            '7', 0x00, 'c', 0x00, '2', 0x00, '3', 0x00,
            'd', 0x00, 'd', 0x00, 'e', 0x00, 'a', 0x00,
            '5', 0x00, '}'
        )
    )
};

}

ControlInterfaceClassDriver::ControlInterfaceClassDriver()
        : ClassDriver() {
}

ControlInterfaceClassDriver* ControlInterfaceClassDriver::instance() {
    static ControlInterfaceClassDriver driver;
    return &driver;
}

ControlInterfaceClassDriver::~ControlInterfaceClassDriver() {
}

int ControlInterfaceClassDriver::init(unsigned cfgIdx) {
    state_ = State::NONE;
    return 0;
}

int ControlInterfaceClassDriver::deinit(unsigned cfgIdx) {
    state_ = State::NONE;
    return 0;
}

int ControlInterfaceClassDriver::handleMsftRequest(SetupRequest* req) {
    if (req->wIndex == 0x0004) {
        /* Extended Compat ID OS Descriptor */
        const uint8_t extendedCompatId[] = {
            USB_WCID_EXT_COMPAT_ID_OS_DESCRIPTOR(
                (uint8_t)interfaceBase_,
                USB_WCID_DATA('W', 'I', 'N', 'U', 'S', 'B', '\0', '\0'),
                USB_WCID_DATA(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
            )
        };
        dev_->setupReply(req, extendedCompatId, sizeof(extendedCompatId));
    } else if (req->wIndex == 0x0005) {
        if ((req->wValue & 0xff) == 0x00) {
            dev_->setupReply(req, MSFT_EXTENDED_PROPERTIES_OS_DESCRIPTOR, sizeof(MSFT_EXTENDED_PROPERTIES_OS_DESCRIPTOR));
        } else {
            // Send dummy
            memset(requestData_, 0, sizeof(requestData_));
            dev_->setupReply(req, requestData_, req->wLength);
        }
    } else {
        dev_->setupError(req);
    }

    return 0;
}

int ControlInterfaceClassDriver::handleVendorRequest(SetupRequest* req) {
    CHECK_TRUE(requestCallback_, SYSTEM_ERROR_INVALID_STATE);

    request_.bmRequestType = req->bmRequestType;
    request_.bRequest = req->bRequest;
    request_.wValue = req->wValue;
    request_.wIndex = req->wIndex;
    request_.wLength = req->wLength;

    int ret = 0;

    if (req->wLength) {
        // Setup request with data stage
        if (req->bmRequestType & SetupRequest::DIRECTION_DEVICE_TO_HOST) {
            // Device -> Host
            state_ = State::TX;
            if (req->wLength <= EP0_MAX_PACKET_SIZE) {
                request_.data = requestData_;
            } else {
                request_.data = nullptr;
            }
            ret = requestCallback_(&request_, requestCallbackCtx_);


            if (ret == 0 && ((request_.data != nullptr && request_.wLength) || !request_.wLength)) {
                if (request_.data != requestData_ &&
                    request_.wLength <= EP0_MAX_PACKET_SIZE && request_.wLength) {
                    // Don't use user buffer if wLength <= USBD_EP0_MAX_PACKET_SIZE
                    // and copy into internal buffer
                    memcpy(requestData_, request_.data, request_.wLength);
                    request_.data = requestData_;
                }
                dev_->setupReply(req, request_.data, request_.wLength);
            } else {
                ret = SYSTEM_ERROR_INTERNAL;
            }
        } else {
            // Host -> Device
            state_ = State::RX;
            if (req->wLength <= EP0_MAX_PACKET_SIZE) {
                // Use internal buffer
                request_.data = requestData_;
                dev_->setupReceive(req, requestData_, req->wLength);
            } else {
                // Try to request the buffer
                request_.data = nullptr;
                requestCallback_(&request_, requestCallbackCtx_);
                if (request_.data != nullptr && request_.wLength >= req->wLength) {
                    request_.wLength = req->wLength;
                    dev_->setupReceive(req, request_.data, req->wLength);
                } else {
                    ret = SYSTEM_ERROR_INTERNAL;
                }
            }
        }
    } else {
        // Setup request without data stage
        request_.data = nullptr;
        ret = requestCallback_(&request_, requestCallbackCtx_);
    }
    return ret == 0 ? ret : SYSTEM_ERROR_INTERNAL;
}

int ControlInterfaceClassDriver::setup(SetupRequest* req) {
    // FIXME: magic numbers
    if ((req->bRequest == 0xee && req->bmRequestType == 0xc1 && req->wIndex == 0x0005) ||
            (req->bRequest == 0xee && req->bmRequestType == 0xc0 && req->wIndex == 0x0004)) {
        return handleMsftRequest(req);
    }

    switch (req->bmRequestTypeType) {
    case SetupRequest::TYPE_VENDOR: {
        return handleVendorRequest(req);
    }
    }

    return SYSTEM_ERROR_INVALID_ARGUMENT;
}

int ControlInterfaceClassDriver::dataIn(unsigned ep, particle::usbd::EndpointEvent ev, size_t len) {
    // Data stage completed
    if (state_ == State::TX && stateCallback_) {
        stateCallback_(HAL_USB_VENDOR_REQUEST_STATE_TX_COMPLETED, stateCallbackCtx_);
        state_ = State::NONE;
        counter_ = 0;
        return 0;
    }
    return SYSTEM_ERROR_INVALID_STATE;
}

int ControlInterfaceClassDriver::dataOut(unsigned ep, particle::usbd::EndpointEvent ev, size_t len) {
    // Data stage completed
    if (state_ == State::RX && requestCallback_) {
        requestCallback_(&request_, requestCallbackCtx_);
        state_ = State::NONE;
        counter_ = 0;
        return 0;
    }
    return SYSTEM_ERROR_INVALID_STATE;
}

int ControlInterfaceClassDriver::startOfFrame() {
    // TODO: counter
    return 0;
}

int ControlInterfaceClassDriver::getConfigurationDescriptor(uint8_t* buf, size_t length, Speed speed, unsigned index) {
    BufferAppender appender(buf, length);
    appender.appendUInt8(0x09);                         /* bLength: Interface Descriptor size */
    appender.appendUInt8(DESCRIPTOR_INTERFACE);         /* bDescriptorType: Interface descriptor type */
    appender.appendUInt8(interfaceBase_);               /* bInterfaceNumber: Number of Interface */
    appender.appendUInt8(0x00);                         /* bAlternateSetting: Alternate setting */
    appender.appendUInt8(0x00);                         /* bNumEndpoints */
    appender.appendUInt8(0xff);                         /* bInterfaceClass: Vendor */
    appender.appendUInt8(0xff);                         /* bInterfaceSubClass */
    appender.appendUInt8(0xff);                         /* nInterfaceProtocol */
    appender.appendUInt8(stringBase_);                  /* iInterface: Index of string descriptor */
    return appender.dataSize();
}

int ControlInterfaceClassDriver::getString(unsigned id, uint16_t langId, uint8_t* buf, size_t length) {
    if (id == stringBase_) {
        const char str[] = HAL_PLATFORM_USB_PRODUCT_STRING " " "Control Interface";
        return dev_->getUnicodeString(str, strlen(str), buf, length);
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int ControlInterfaceClassDriver::getNumInterfaces() const {
    return 1;
}

int ControlInterfaceClassDriver::getNumStrings() const {
    return 1;
}

unsigned ControlInterfaceClassDriver::getEndpointMask() const {
    return 0;
}

void ControlInterfaceClassDriver::setVendorRequestCallback(HAL_USB_Vendor_Request_Callback cb, void* ptr) {
    requestCallback_ = cb;
    requestCallbackCtx_ = ptr;
}

void ControlInterfaceClassDriver::setVendorRequestStateCallback(HAL_USB_Vendor_Request_State_Callback cb, void* p) {
    stateCallback_ = cb;
    stateCallbackCtx_ = p;
}
