/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#include "logging.h"
#include "usbd_hid.h"
#include <cstring>
#include <algorithm>
#include "usbd_wcid.h"
#include "appender.h"
#include "check.h"
#include "scope_guard.h"
#include "endian_util.h"
#include <algorithm>
#include <mutex>
#include "service_debug.h"

using namespace particle::usbd;

namespace {

const char DEFAULT_NAME[] = HAL_PLATFORM_USB_PRODUCT_STRING " " "HID Mouse / Keyboard";

uint8_t __attribute__((aligned(4))) DEFAULT_HID_DESCRIPTOR[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x01,                    //   REPORT_ID (1)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    // 0x36, 0x01, 0x80,              //     PHYSICAL_MINIMUM (-32767)
    // 0x46, 0xff, 0x7f,              //     PHYSICAL_MAXIMUM (32767)
    0x16, 0x01, 0x80,              //     LOGICAL_MINIMUM (-32767)
    0x26, 0xff, 0x7f,              //     LOGICAL_MAXIMUM (32767)
    // 0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    // 0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x10,                    //     REPORT_SIZE (16)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0x09, 0x38,                    //     USAGE (WHEEL)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0xc0,                          //   END_COLLECTION
    0xc0,                          // END_COLLECTION
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x02,                    //   REPORT_ID (2)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x01,                    //   INPUT (Cnst,Ary,Abs)
    // We don't use OUT endpoint to receive LED reports from host
    // 0x95, 0x05,                    //   REPORT_COUNT (5)
    // 0x75, 0x01,                    //   REPORT_SIZE (1)
    // 0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    // 0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    // 0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
    // 0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    // 0x95, 0x01,                    //   REPORT_COUNT (1)
    // 0x75, 0x03,                    //   REPORT_SIZE (3)
    // 0x91, 0x01,                    //   OUTPUT (Cnst,Ary,Abs)
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0xdd,                    //   LOGICAL_MAXIMUM (221)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0xdd,                    //   USAGE_MAXIMUM (Keypad Hexadecimal)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0,                          // END_COLLECTION
    0x05, 0x0d,                    // USAGE_PAGE (Digitizer)
    0x09, 0x02,                    // USAGE (Pen)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x03,                    //   REPORT_ID (3)
    0x09, 0x20,                    //   USAGE (Stylus)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x09, 0x42,                    //     USAGE (Tip Switch)
    0x09, 0x32,                    //     USAGE (In Rnage)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x06,                    //     REPORT_COUNT (6)
    0x81, 0x01,                    //     INPUT (Cnst,Ary,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    //     USAGE (Pointer)
    0xa1, 0x00,                    //     COLLECTION (Physical)
    0x09, 0x30,                    //       USAGE (X)
    0x09, 0x31,                    //       USAGE (Y)
    0x16, 0x00, 0x00,              //       LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x7f,              //       LOGICAL_MAXIMUM (32767)
    0x36, 0x00, 0x00,              //       PHYSICAL_MINIMUM (0)
    0x46, 0xff, 0x7f,              //       PHYSICAL_MAXIMUM (32767)
    0x65, 0x00,                    //       UNIT (None)
    0x75, 0x10,                    //       REPORT_SIZE (16)
    0x95, 0x02,                    //       REPORT_COUNT (2)
    0x81, 0x02,                    //       INPUT (Data,Var,Abs)
    0xc0,                          //     END_COLLECTION
    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};

const size_t DEFAULT_HID_DESCRIPTOR_DIGITIZER_LEN = 65;

} // anonymous

HidClassDriver::HidClassDriver()
        : ClassDriver() {
}

HidClassDriver::~HidClassDriver() {
    setName(nullptr);
}

HidClassDriver* HidClassDriver::instance() {
    static HidClassDriver hid;
    return &hid;
}

int HidClassDriver::init(unsigned cfgIdx) {
    // deinit(cfgIdx);
    CHECK_TRUE(epIn_ != ENDPOINT_INVALID, SYSTEM_ERROR_INVALID_STATE);

    CHECK(dev_->openEndpoint(epIn_, EndpointType::INTERRUPT, hid::MAX_DATA_PACKET_SIZE));

    return 0;
}

int HidClassDriver::deinit(unsigned cfgIdx) {
    if (isConfigured()) {
        if (txState_) {
            dev_->flushEndpoint(epIn_);
        }
    }

    dev_->closeEndpoint(epIn_);

    epIn_ = ENDPOINT_INVALID;
    txState_ = false;

    altSetting_ = 0;
    protocol_ = 0;
    idleState_ = 0;
    return 0;
}

void HidClassDriver::setName(const char* name) {
    if (name_) {
        free(name_);
        name_ = nullptr;
    }
    if (name) {
        // Ignore allocation failures, we'll just default to DEFAULT_NAME
        name_ = strdup(name);
    }
}

int HidClassDriver::setup(SetupRequest* req) {
    if (req->bmRequestTypeType != SetupRequest::TYPE_CLASS && req->bmRequestTypeType != SetupRequest::TYPE_STANDARD) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (req->wIndex != interfaceBase_) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    // Class or standard request
    if (req->bmRequestTypeType == SetupRequest::TYPE_CLASS) {
        if (req->bmRequestTypeDirection /* Device to host */) {
            return handleInSetupRequest(req);
        } else {
            return handleOutSetupRequest(req);
        }
    } else {
        // Standard
        switch (req->bRequest) {
        case SetupRequest::REQUEST_GET_DESCRIPTOR: {
            if ((req->wValue >> 8) == hid::DESCRIPTOR_HID_REPORT) {
                return dev_->setupReply(req, DEFAULT_HID_DESCRIPTOR, reportDescriptorLen());
            }
            break;
        }
        case SetupRequest::REQUEST_GET_INTERFACE: {
            return dev_->setupReply(req, &altSetting_, sizeof(altSetting_));
        }
        case SetupRequest::REQUEST_SET_INTERFACE: {
            if (req->wValue == 0) {
                return dev_->setupReply(req, nullptr, 0);
            }
            break;
        }
        }
    }
    return SYSTEM_ERROR_INVALID_ARGUMENT;
}

int HidClassDriver::handleInSetupRequest(SetupRequest* req) {
    switch (req->bRequest) {
    case hid::GET_PROTOCOL: {
        return dev_->setupReply(req, (const uint8_t*)&protocol_, sizeof(protocol_));
    }
    case hid::GET_IDLE: {
        return dev_->setupReply(req, (const uint8_t*)&idleState_, sizeof(idleState_));
    }
    }
    return SYSTEM_ERROR_INVALID_ARGUMENT;
}

int HidClassDriver::handleOutSetupRequest(SetupRequest* req) {
    switch (req->bRequest) {
    case hid::SET_PROTOCOL: {
        protocol_ = (uint8_t)req->wValue;
        return 0;
    }
    case hid::SET_IDLE: {
        idleState_ = (uint8_t)req->wValue;
        return 0;
    }
    }
    return SYSTEM_ERROR_INVALID_ARGUMENT;
}

int HidClassDriver::dataIn(unsigned ep, particle::usbd::EndpointEvent ev, size_t len) {
    if (ep != epIn_) {
        return SYSTEM_ERROR_UNKNOWN;
    }

    if (!txState_) {
        return 0;
    }

    txState_ = false;

    return 0;
}

int HidClassDriver::dataOut(unsigned ep, particle::usbd::EndpointEvent ev, size_t len) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int HidClassDriver::startOfFrame() {
    return 0;
}

int HidClassDriver::getConfigurationDescriptor(uint8_t* buf, size_t length, Speed speed, unsigned index) {
    CHECK_TRUE(buf && length, SYSTEM_ERROR_INVALID_ARGUMENT);
    BufferAppender appender(buf, length);
 
    // No need for interface association descriptor, as only one interface is used
    
    // Interface Descriptor
    appender.appendUInt8(0x09);   // bLength: Interface Descriptor size
    appender.appendUInt8(DESCRIPTOR_INTERFACE);  // bDescriptorType: Interface
    appender.appendUInt8(interfaceBase_);   // bInterfaceNumber: Number of Interface
    appender.appendUInt8(0x00);   // bAlternateSetting: Alternate setting
    appender.appendUInt8(0x01);   // bNumEndpoints: 1, we are out of endpoints
    appender.appendUInt8(0x03);   // bInterfaceClass: HID
    appender.appendUInt8(0x00);   // bInterfaceSubClass: 0 = BOOT
    appender.appendUInt8(0x00);   // bInterfaceProtocol: 0 = none
    appender.appendUInt8(stringBase_);   // iInterface (using same string as iFunction in IAD)

    // Joystick Mouse HID
    appender.appendUInt8(0x09); // bLength: HID descriptor size
    appender.appendUInt8(0x21); // bDescriptorType: HID
    appender.appendUInt8(0x11); // bcdHID: HID Class Spec release number
    appender.appendUInt8(0x01); //
    appender.appendUInt8(0x00); // bCountryCode: Hardware target country
    appender.appendUInt8(0x01); // bNumDescriptors: Number of HID class descriptors to follow
    appender.appendUInt8(0x22); // bDescriptorType: HID
    appender.appendUInt16LE(reportDescriptorLen()); // wItemLength: Total length of Report descriptor

    uint8_t epIn = CHECK(getNextAvailableEndpoint(ENDPOINT_IN));
    // Endpoint descriptor (IN)
    appender.appendUInt8(0x07); // bLength: Endpoint Descriptor size
    appender.appendUInt8(DESCRIPTOR_ENDPOINT); // bDescriptorType
    appender.appendUInt8(epIn); // bEndpointAddress: Endpoint Address (IN)
    appender.appendUInt8(0x03); // bmAttributes: Interrupt endpoint
    appender.appendUInt16LE(hid::MAX_DATA_PACKET_SIZE); // wMaxPacketSize
    appender.appendUInt8(0x01); // bInterval: Polling Interval (10 ms)

    epIn_ = epIn;

    return appender.dataSize();
}

int HidClassDriver::getString(unsigned id, uint16_t langId, uint8_t* buf, size_t length) {
    if (id == stringBase_) {
        if (name_) {
            return dev_->getUnicodeString(name_, strlen(name_), buf, length);
        }
        return dev_->getUnicodeString(DEFAULT_NAME, sizeof(DEFAULT_NAME) - 1, buf, length);
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int HidClassDriver::getNumInterfaces() const {
    return 1;
}

int HidClassDriver::getNumStrings() const {
    return 1;
}

int HidClassDriver::sendReport(const uint8_t* report, size_t size) {
    CHECK_TRUE(isConfigured(), SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(report && size, SYSTEM_ERROR_INVALID_ARGUMENT);

    if (txState_) {
        return SYSTEM_ERROR_BUSY;
    }

    if (size > hid::MAX_DATA_PACKET_SIZE) {
        return SYSTEM_ERROR_TOO_LARGE;
    }

    txState_ = true;

    memcpy(buffer_, report, size);
    return dev_->transferIn(epIn_, buffer_, size);
}

int HidClassDriver::setDigitizerState(bool state) {
    digitizerState_ = state;
    return 0;
}

int HidClassDriver::transferStatus() const {
    return txState_ && isConfigured();
}

size_t HidClassDriver::reportDescriptorLen() const {
    size_t len = sizeof(DEFAULT_HID_DESCRIPTOR);
    if (!digitizerState_) {
        len -= DEFAULT_HID_DESCRIPTOR_DIGITIZER_LEN;
    }
    return len;
}