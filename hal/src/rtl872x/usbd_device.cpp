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

#include "usbd_device.h"
#include "usbd_driver.h"
#include "system_error.h"
#include "service_debug.h"
#include "check.h"
#include "deviceid_hal.h"
#include "bytes2hexbuf.h"
#include "usbd_wcid.h"
#include <algorithm>
#include "appender.h"

using namespace particle::usbd;

namespace {

enum ConfigurationIndex {
    CONFIGURATION_NONE = 0,
    CONFIGURATION_500MA = 1,
    CONFIGURATION_100MA = 2
};

enum MaxPower {
    BMAXPOWER_100MA = 0x32,
    BMAXPOWER_500MA = 0xFA
};

char* device_id_as_string(char* buf) {
    uint8_t deviceId[HAL_DEVICE_ID_SIZE] = {};
    unsigned deviceIdLen = hal_get_device_id(deviceId, sizeof(deviceId));
    bytes2hexbuf_lower_case(deviceId, deviceIdLen, buf);
    return buf;
}

/* MS OS String Descriptor */
const uint8_t MSFT_STR_DESC[] = {
    USB_WCID_MS_OS_STRING_DESCRIPTOR(
        // "MSFT100"
        USB_WCID_DATA('M', '\0', 'S', '\0', 'F', '\0', 'T', '\0', '1', '\0', '0', '\0', '0', '\0'),
        0xee
    )
};


} // anonymous

int Device::registerDriver(DeviceDriver* driver) {
    driver_ = driver;
    if (driver) {
        driver->setDeviceInstance(this);
    }
    return 0;
}

int Device::registerClass(ClassDriver* drv) {
    for (auto& cls: classDrivers_) {
        if (!cls) {
            cls = drv;
            drv->setDeviceInstance(this);
            return 0;
        }
    }
    return SYSTEM_ERROR_LIMIT_EXCEEDED;
}

int Device::removeClass(ClassDriver* drv) {
    for (auto& cls: classDrivers_) {
        if (cls && cls == drv) {
            cls = nullptr;
            return 0;
        }
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int Device::setDeviceDescriptor(const uint8_t* desc, size_t len) {
    deviceDescriptorLen_ = std::min<size_t>(len, sizeof(deviceDescriptor_));
    memcpy(deviceDescriptor_, desc, deviceDescriptorLen_);
    return deviceDescriptorLen_;
}

int Device::setDriverInstance(DeviceDriver* driver) {
    driver_ = driver;
    if (driver) {
        driver->setDeviceInstance(this);
    }
    return 0;
}

DeviceState Device::getDeviceState() const {
    if (driver_) {
        return driver_->getState();
    }
    return DeviceState::NONE;
}

bool Device::lock() {
    if (driver_) {
        return driver_->lock();
    }
    return false;
}

void Device::unlock() {
    if (driver_) {
        driver_->unlock();
    }
}

unsigned Device::updateEndpointMask(unsigned mask) {
    if (driver_) {
        return driver_->updateEndpointMask(mask);
    }
    return mask;
}

int Device::attach() {
    CHECK_TRUE(driver_, SYSTEM_ERROR_INVALID_STATE);
    CHECK(driver_->attach());
    return 0;
}

int Device::detach() {
    CHECK_TRUE(driver_, SYSTEM_ERROR_INVALID_STATE);
    CHECK(driver_->detach());
    return 0;
}

int Device::suspend() {
    CHECK_TRUE(driver_, SYSTEM_ERROR_INVALID_STATE);
    CHECK(driver_->suspend());
    return 0;
}

int Device::resume() {
    CHECK_TRUE(driver_, SYSTEM_ERROR_INVALID_STATE);
    CHECK(driver_->resume());
    return 0;
}

int Device::openEndpoint(unsigned ep, EndpointType type, size_t pakSize) {
    CHECK_TRUE(driver_, SYSTEM_ERROR_INVALID_STATE);
    return driver_->openEndpoint(ep, type, pakSize);
}

int Device::closeEndpoint(unsigned ep) {
    CHECK_TRUE(driver_, SYSTEM_ERROR_INVALID_STATE);
    return driver_->closeEndpoint(ep);
}

int Device::flushEndpoint(unsigned ep) {
    CHECK_TRUE(driver_, SYSTEM_ERROR_INVALID_STATE);
    return driver_->flushEndpoint(ep);
}

int Device::stallEndpoint(unsigned ep) {
    CHECK_TRUE(driver_, SYSTEM_ERROR_INVALID_STATE);
    return driver_->stallEndpoint(ep);
}

int Device::clearStallEndpoint(unsigned ep) {
    CHECK_TRUE(driver_, SYSTEM_ERROR_INVALID_STATE);
    return driver_->clearStallEndpoint(ep);
}

EndpointStatus Device::getEndpointStatus(unsigned ep) {
    if (driver_) {
        return driver_->getEndpointStatus(ep);
    }
    return EndpointStatus::NONE;
}

int Device::setEndpointStatus(unsigned ep, EndpointStatus status) {
    if (driver_) {
        return driver_->setEndpointStatus(ep, status);
    }
    return SYSTEM_ERROR_INVALID_STATE;
}

int Device::transferIn(unsigned ep, const uint8_t* ptr, size_t size) {
    CHECK_TRUE(driver_, SYSTEM_ERROR_INVALID_STATE);
    return driver_->transferIn(ep, ptr, size);
}

int Device::transferOut(unsigned ep, uint8_t* ptr, size_t size) {
    CHECK_TRUE(driver_, SYSTEM_ERROR_INVALID_STATE);
    return driver_->transferOut(ep, ptr, size);
}

int Device::setupReply(SetupRequest* r, const uint8_t* data, size_t size) {
    CHECK_TRUE(driver_, SYSTEM_ERROR_INVALID_STATE);
    return driver_->setupReply(r, data, size);
}

int Device::setupReceive(SetupRequest* r, uint8_t* data, size_t size) {
    CHECK_TRUE(driver_, SYSTEM_ERROR_INVALID_STATE);
    return driver_->setupReceive(r, data, size);
}

int Device::setupError(SetupRequest* r) {
    CHECK_TRUE(driver_, SYSTEM_ERROR_INVALID_STATE);
    return driver_->setupError(r);
}


int Device::getDescriptor(DescriptorType type, uint8_t* buf, size_t len, Speed speed, unsigned index) {
    switch (type) {
    case DESCRIPTOR_DEVICE:
    // Use same config
    case DESCRIPTOR_OTHER_SPEED_CONFIGURATION: {
        auto sz = std::min(deviceDescriptorLen_, len);
        if (buf) {
            memcpy(buf, deviceDescriptor_, sz);
        }
        return sz;
    }
    case DESCRIPTOR_CONFIGURATION: {
        size_t pos = 0;
#if HAL_PLATFORM_USB_COMPOSITE
        pos += 0x09; // Skip composite header
#endif // HAL_PLATFORM_USB_COMPOSITE
        unsigned baseInterface = 0;
        unsigned baseString = STRING_IDX_INTERFACE;
        CHECK_TRUE(driver_, SYSTEM_ERROR_INVALID_STATE);
        unsigned epMask = ~(driver_->getEndpointMask());
        for (auto& cls: classDrivers_) {
            if (cls && cls->isEnabled()) {
                cls->setBaseIndices(baseInterface, baseString, epMask);
                int r = cls->getConfigurationDescriptor(buf ? (buf + pos) : nullptr, len - pos, speed, index);
                if (r >= 0) {
                    pos += r;
                    baseInterface += cls->getNumInterfaces();
                    baseString += cls->getNumStrings();
                    auto mask = cls->getEndpointMask();
                    epMask |= mask;
                }
            }
        }
        if (buf) {
#if HAL_PLATFORM_USB_COMPOSITE
            index++;
            particle::BufferAppender appender(buf, 0x09);
            appender.appendUInt8(0x09);                     /* bLength */
            appender.appendUInt8(DESCRIPTOR_CONFIGURATION); /* bDescriptorType */
            appender.appendUInt16LE(pos);                   /* wTotalLength */
            appender.appendUInt8(baseInterface);            /* bNumInterfaces */
            appender.appendUInt8(index);                    /* bConfigurationValue */
            appender.appendUInt8(STRING_IDX_CONFIG);        /* iConfiguration */
            appender.appendUInt8(0x80);                     /* bmAttirbutes (Bus powered) */
            if (index != CONFIGURATION_100MA) {
                appender.appendUInt8(BMAXPOWER_500MA);      /* bMaxPower (500mA) */
            } else {
                appender.appendUInt8(BMAXPOWER_100MA);
            }
#endif // HAL_PLATFORM_USB_COMPOSITE
        }
        return pos;
    }
    case DESCRIPTOR_DEVICE_QUALIFIER: {
        particle::BufferAppender appender(buf, 0x09);
        appender.appendUInt8(0x09);
        appender.appendUInt8(DESCRIPTOR_DEVICE_QUALIFIER);
        appender.appendUInt8(0x00);
        appender.appendUInt8(0x02);
        appender.appendUInt8(0x00);
        appender.appendUInt8(0x00);
        appender.appendUInt8(0x40);
        appender.appendUInt8(0x01);
        appender.appendUInt8(0x00);
        break;
    }
    }
    return SYSTEM_ERROR_INVALID_ARGUMENT;
}

int Device::getUnicodeString(const char* ascii, size_t len, uint8_t* buf, size_t buflen) {
    if (ascii == nullptr || len == 0) {
        return 0;
    }

    if (buflen < (len + 2) * 2) {
        return SYSTEM_ERROR_TOO_LARGE;
    }

    if (buf) {
        buf[0] = (uint8_t)(len * 2 + 2);
        buf[1] = DESCRIPTOR_STRING;

        for (size_t i = 0; i < len; i++) {
            buf[2 + (i * 2)] = ascii[i];
            buf[2 + (i * 2) + 1] = 0x00;
        }
    }

    return len * 2 + 2;
}

int Device::getRawString(const char* data, size_t len, uint8_t* buf, size_t buflen) {
    if (data == nullptr || len == 0) {
        return 0;
    }

    if (buflen < (len + 2)) {
        return SYSTEM_ERROR_TOO_LARGE;
    }

    if (buf) {
        buf[0] = (uint8_t)(len + 2);
        buf[1] = DESCRIPTOR_STRING;

        for (size_t i = 0; i < len; i++) {
            buf[2 + i] = data[i];
        }
    }

    return len + 2;
}

int Device::getString(unsigned id, uint16_t langId, uint8_t* buf, size_t len) {
    switch (id) {
    case STRING_IDX_LANGID: {
        return getRawString(HAL_PLATFORM_USB_LANGID_LIST, sizeof(HAL_PLATFORM_USB_LANGID_LIST) - 1, buf, len);
    }
    case STRING_IDX_MANUFACTURER: {
        return getUnicodeString(HAL_PLATFORM_USB_MANUFACTURER_STRING, sizeof(HAL_PLATFORM_USB_MANUFACTURER_STRING) - 1, buf, len);
    }
    case STRING_IDX_PRODUCT: {
        return getUnicodeString(HAL_PLATFORM_USB_PRODUCT_STRING, sizeof(HAL_PLATFORM_USB_PRODUCT_STRING) - 1, buf, len);
    }
    case STRING_IDX_SERIAL: {
        char deviceid[HAL_DEVICE_ID_SIZE * 2] = {};
        return getUnicodeString(device_id_as_string(deviceid), sizeof(deviceid), buf, len);
    }
    case STRING_IDX_CONFIG: {
        return getUnicodeString(HAL_PLATFORM_USB_CONFIGURATION_STRING, sizeof(HAL_PLATFORM_USB_CONFIGURATION_STRING) - 1, buf, len);
    }
    case STRING_IDX_MSFT: {
        return getRawString((const char*)MSFT_STR_DESC, sizeof(MSFT_STR_DESC), buf, len);
    }
    default: {
        for (auto& cls: classDrivers_) {
            if (cls && cls->isEnabled()) {
                int r = cls->getString(id, langId, buf, len);
                if (r >= 0) {
                    return r;
                }
            }
        }
        break;
    }
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int Device::notifyReset() {
    return 0;
}

int Device::notifySuspend() {
    return 0;
}

int Device::notifyResume() {
    return 0;
}

int Device::setConfig(unsigned index) {
    int lastError = 0;
    for (auto& cls: classDrivers_) {
        if (cls && cls->isEnabled()) {
            int r = cls->init(index);
            if (r < 0) {
                lastError = r;
                cls->deinit(index);
            } else {
                cls->configured(true);
            }
        }
    }
    return lastError;
}

int Device::clearConfig(unsigned index) {
    int lastError = 0;
    for (auto& cls: classDrivers_) {
        if (cls && cls->isEnabled()) {
            if (cls->isConfigured()) {
                int r = cls->deinit(index);
                if (r < 0) {
                    lastError = r;
                }
                cls->configured(false);
            }
        }
    }
    return lastError;
}

int Device::startOfFrame() {
    for (auto& cls: classDrivers_) {
        if (cls && cls->isEnabled()) {
            cls->startOfFrame();
        }
    }
    return 0;
}

int Device::setupRequest(SetupRequest* req) {
    for (auto& cls: classDrivers_) {
        if (cls && cls->isEnabled()) {
            int r = cls->setup(req);
            if (r >= 0) {
                return r;
            }
        }
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int Device::notifyEpTransferDone(unsigned ep, EndpointEvent event, size_t len) {
    for (auto& cls: classDrivers_) {
        if (cls && cls->isEnabled()) {
            int r;
            if (ep & SetupRequest::DIRECTION_DEVICE_TO_HOST) {
                r = cls->dataIn(ep, event, len);
            } else {
                r = cls->dataOut(ep, event, len);
            }
            if (r >= 0) {
                return r;
            }
        }
    }
    if (ep & 0x7f) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    return 0;
}

int Device::notifyDeviceState(DeviceState state) {
    return 0;
}

// DeviceDriver

void DeviceDriver::setDeviceInstance(Device* device) {
    dev_ = device;
}

int DeviceDriver::getDescriptor(DescriptorType type, uint8_t* buf, size_t len, Speed speed, unsigned index) {
    CHECK_TRUE(dev_, SYSTEM_ERROR_INVALID_STATE);
    return dev_->getDescriptor(type, buf, len, speed, index);
}

int DeviceDriver::getString(unsigned id, uint16_t langId, uint8_t* buf, size_t len) {
    CHECK_TRUE(dev_, SYSTEM_ERROR_INVALID_STATE);
    return dev_->getString(id, langId, buf, len);
}

int DeviceDriver::notifyReset() {
    CHECK_TRUE(dev_, SYSTEM_ERROR_INVALID_STATE);
    return dev_->notifyReset();
}

int DeviceDriver::notifySuspend() {
    CHECK_TRUE(dev_, SYSTEM_ERROR_INVALID_STATE);
    return dev_->notifySuspend();
}

int DeviceDriver::notifyResume() {
    CHECK_TRUE(dev_, SYSTEM_ERROR_INVALID_STATE);
    return dev_->notifyResume();
}

int DeviceDriver::setConfig(unsigned index) {
    CHECK_TRUE(dev_, SYSTEM_ERROR_INVALID_STATE);
    return dev_->setConfig(index);
}

int DeviceDriver::clearConfig(unsigned index) {
    CHECK_TRUE(dev_, SYSTEM_ERROR_INVALID_STATE);
    return dev_->clearConfig(index);
}

int DeviceDriver::startOfFrame() {
    CHECK_TRUE(dev_, SYSTEM_ERROR_INVALID_STATE);
    return dev_->startOfFrame();
}

int DeviceDriver::setupRequest(SetupRequest* req) {
    CHECK_TRUE(dev_, SYSTEM_ERROR_INVALID_STATE);
    return dev_->setupRequest(req);
}

int DeviceDriver::notifyEpTransferDone(unsigned ep, EndpointEvent event, size_t len) {
    CHECK_TRUE(dev_, SYSTEM_ERROR_INVALID_STATE);
    return dev_->notifyEpTransferDone(ep, event, len);
}

int DeviceDriver::notifyDeviceState(DeviceState state) {
    CHECK_TRUE(dev_, SYSTEM_ERROR_INVALID_STATE);
    return dev_->notifyDeviceState(state);
}

// ClassDriver

ClassDriver::ClassDriver() {
}

ClassDriver::ClassDriver(Device* dev)
        : dev_(dev) {
}

void ClassDriver::setDeviceInstance(Device* dev) {
    dev_ = dev;
}

void ClassDriver::enable(bool state) {
    enabled_ = state;
}

bool ClassDriver::isEnabled() const {
    return enabled_;
}

void ClassDriver::configured(bool state) {
    configured_ = state;
}

bool ClassDriver::isConfigured() const {
    return configured_;
}

int ClassDriver::setBaseIndices(unsigned interface, unsigned string, unsigned epMask) {
    interfaceBase_ = interface;
    stringBase_ = string;
    endpointMask_ = epMask;
    return 0;
}

int ClassDriver::getNextAvailableEndpoint(EndpointDirection type, bool reserve) {
    unsigned base = 1;
    if (type == ENDPOINT_IN) {
        base <<= ENDPOINT_MASK_OUT_BITPOS;
    }
    for (unsigned pos = 0; pos < ENDPOINT_MASK_OUT_BITPOS; pos++) {
        bool epBit = endpointMask_ & (base << pos);
        if (!epBit) {
            // Available
            if (reserve) {
                endpointMask_ |= (base << pos);
                endpointMask_ = dev_->updateEndpointMask(endpointMask_);
            }
            // Endpoint 0 is default, not counted here
            return ((pos + 1) | (int)type);
        }
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

unsigned ClassDriver::getEndpointMask() const {
    return endpointMask_;
}
