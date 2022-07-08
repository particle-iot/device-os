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

#pragma once

#include <cstddef>
#include <cstdint>
#include "hal_platform.h"

namespace particle { namespace usbd {

/* Forward declaration */
class ClassDriver;
class Device;
class DeviceDriver;

struct SetupRequest {
    union {
        uint8_t bmRequestType;
        struct {
            uint8_t bmRequestTypeRecipient : 5;
            uint8_t bmRequestTypeType : 2;
            uint8_t bmRequestTypeDirection : 1;
        };
    };
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;

    enum Type {
        TYPE_STANDARD = 0x00,
        TYPE_CLASS = 0x01,
        TYPE_VENDOR = 0x02
    };

    enum Recipient {
        RECIPIENT_DEVICE = 0x00,
        RECIPIENT_INTERFACE = 0x01,
        RECIPIENT_ENDPOINT = 0x02,
        RECIPIENT_OTHER = 0x03
    };

    enum StandardRequest {
        REQUEST_GET_STATUS = 0x00,
        REQUEST_CLEAR_FEATURE = 0x01,
        REQUEST_SET_FEATURE = 0x03,
        REQUEST_SET_ADDRESS = 0x05,
        REQUEST_GET_DESCRIPTOR = 0x06,
        REQUEST_SET_DESCRIPTOR = 0x07,
        REQUEST_GET_CONFIGURATION = 0x08,
        REQUEST_SET_CONFIGURATION = 0x09,
        REQUEST_GET_INTERFACE = 0x0a,
        REQUEST_SET_INTERFACE = 0x0b,
        REQUEST_SYNCH_FRAME = 0x0c
    };

    enum TransferDirection {
        DIRECTION_HOST_TO_DEVICE = 0x00,
        DIRECTION_DEVICE_TO_HOST = 0x80
    };
};

const size_t MAX_COMPOSITE_CLASS_DRIVERS = 5;

enum class EndpointStatus {
    NONE = 0,
    OK,
    DISABLED,
    STALL,
    NAK
};

enum class DeviceState {
    NONE = 0,
    DISABLED,
    DETACHED,
    ATTACHED,
    POWERED,
    DEFAULT,
    ADDRESSED,
    CONFIGURED,
    SUSPENDED
};

enum DescriptorType {
    DESCRIPTOR_DEVICE = 0x01,
    DESCRIPTOR_CONFIGURATION = 0x02,
    DESCRIPTOR_STRING = 0x03,
    DESCRIPTOR_INTERFACE = 0x04,
    DESCRIPTOR_ENDPOINT = 0x05,
    DESCRIPTOR_DEVICE_QUALIFIER = 0x06,
    DESCRIPTOR_OTHER_SPEED_CONFIGURATION = 0x07,
    DESCRIPTOR_OTG = 0x09
};

enum StringIndex {
    STRING_IDX_LANGID = 0x00,
    STRING_IDX_MANUFACTURER = 0x01,
    STRING_IDX_PRODUCT = 0x02,
    STRING_IDX_SERIAL = 0x03,
    STRING_IDX_CONFIG = 0x04,
    STRING_IDX_INTERFACE = 0x05,
    STRING_IDX_MSFT = 0xee
};

enum DeviceFeature {
    FEATURE_ENDPOINT_HALT = 0x00,
    FEATURE_DEVICE_REMOTE_WAKEUP = 0x01,
    FEATURE_DEVICE_TEST_MODE = 0x02
};

enum class EndpointEvent {
    DONE = 0,
    WAIT = 1,
};

enum class Speed {
    FULL = 0,
    HIGH = 1,
    LOW = 2
};

enum class EndpointType {
    CONTROL = 0,
    INTERRUPT,
    ISOCHRONOUS,
    BULK
};

enum EndpointDirection {
    ENDPOINT_OUT = 0x00,
    ENDPOINT_IN = 0x80
};

const unsigned ENDPOINT_MASK_OUT_BITPOS = 16;
const unsigned EP0_MAX_PACKET_SIZE = 64;
const uint8_t ENDPOINT_INVALID = 0xff;

class DeviceDriver {
public:
    virtual ~DeviceDriver() = default;

    virtual int attach() = 0;
    virtual int detach() = 0;
    virtual int suspend() = 0;
    virtual int resume() = 0;
    virtual DeviceState getState() const = 0;

    virtual int openEndpoint(unsigned ep, EndpointType type, size_t pakSize) = 0;
    virtual int closeEndpoint(unsigned ep) = 0;
    virtual int flushEndpoint(unsigned ep) = 0;
    virtual int stallEndpoint(unsigned ep) = 0;
    virtual int clearStallEndpoint(unsigned ep) = 0;
    virtual EndpointStatus getEndpointStatus(unsigned ep) = 0;
    virtual int setEndpointStatus(unsigned ep, EndpointStatus status) = 0;

    virtual int transferIn(unsigned ep, const uint8_t* ptr, size_t size) = 0;
    virtual int transferOut(unsigned ep, uint8_t* ptr, size_t size) = 0;

    virtual int setupReply(SetupRequest* r, const uint8_t* data, size_t size) = 0;
    virtual int setupReceive(SetupRequest* r, uint8_t* data, size_t size) = 0;
    virtual int setupError(SetupRequest* r) = 0;

    void setDeviceInstance(Device* device);

    virtual unsigned getEndpointMask() const = 0;
    virtual unsigned updateEndpointMask(unsigned mask) const = 0;

    virtual bool lock() = 0;
    virtual void unlock() = 0;

protected:
    // Proxied methods to Device instance
    int getDescriptor(DescriptorType type, uint8_t* buf, size_t len, Speed speed = Speed::FULL, unsigned index = 0);
    int getString(unsigned id, uint16_t langId, uint8_t* buf, size_t len);
    int notifyReset();
    int notifySuspend();
    int notifyResume();
    int setConfig(unsigned index);
    int clearConfig(unsigned index);
    int startOfFrame();
    int setupRequest(SetupRequest* req);
    int notifyEpTransferDone(unsigned ep, EndpointEvent event, size_t len);
    int notifyDeviceState(DeviceState state);

    Device* dev_ = nullptr;
};

class Device {
public:
    Device() = default;
    Device(const Device&) = delete;

    int attach();
    int detach();
    int suspend();
    int resume();

    int registerDriver(DeviceDriver* driver);
    int registerClass(ClassDriver* drv);
    int removeClass(ClassDriver* drv);
    int setDeviceDescriptor(const uint8_t* desc, size_t len);
    int setDriverInstance(DeviceDriver* driver);

    DeviceState getDeviceState() const;

    int openEndpoint(unsigned ep, EndpointType type, size_t pakSize);
    int closeEndpoint(unsigned ep);
    int flushEndpoint(unsigned ep);
    int stallEndpoint(unsigned ep);
    int clearStallEndpoint(unsigned ep);
    EndpointStatus getEndpointStatus(unsigned ep);
    int setEndpointStatus(unsigned ep, EndpointStatus status);

    int transferIn(unsigned ep, const uint8_t* ptr, size_t size);
    int transferOut(unsigned ep, uint8_t* ptr, size_t size);

    int setupReply(SetupRequest* r, const uint8_t* data, size_t size);
    int setupReceive(SetupRequest* r, uint8_t* data, size_t size);
    int setupError(SetupRequest* r);

    static int getUnicodeString(const char* ascii, size_t len, uint8_t* buf, size_t buflen);
    static int getRawString(const char* data, size_t len, uint8_t* buf, size_t buflen);

    bool lock();
    void unlock();

    unsigned updateEndpointMask(unsigned mask);

protected:
    friend class DeviceDriver;
    virtual int getDescriptor(DescriptorType type, uint8_t* buf, size_t len, Speed speed = Speed::FULL, unsigned index = 0);
    virtual int getString(unsigned id, uint16_t langId, uint8_t* buf, size_t len);
    int notifyReset();
    int notifySuspend();
    int notifyResume();
    int setConfig(unsigned index);
    int clearConfig(unsigned index);
    int startOfFrame();
    int setupRequest(SetupRequest* req);
    int notifyEpTransferDone(unsigned ep, EndpointEvent event, size_t len);
    int notifyDeviceState(DeviceState state);

private:
    DeviceDriver* driver_ = nullptr;
    ClassDriver* classDrivers_[MAX_COMPOSITE_CLASS_DRIVERS] = {};
    uint8_t deviceDescriptor_[HAL_PLATFORM_USB_MAX_DEVICE_DESCRIPTOR_LEN] = {};
    size_t deviceDescriptorLen_ = 0;
};

class ClassDriver {
public:
    ClassDriver();
    ClassDriver(Device* dev);
    virtual ~ClassDriver() = default;

    void setDeviceInstance(Device* dev);

    virtual int init(unsigned cfgIdx) = 0;
    virtual int deinit(unsigned cfgIdx) = 0;
    virtual int setup(SetupRequest* req) = 0;
    virtual int startOfFrame() = 0;
    virtual int dataIn(unsigned ep, EndpointEvent ev, size_t len) = 0;
    virtual int dataOut(unsigned ep, EndpointEvent ev, size_t len) = 0;
    virtual int getConfigurationDescriptor(uint8_t* buf, size_t length, Speed speed = Speed::FULL, unsigned index = 0) = 0;
    virtual int getString(unsigned id, uint16_t langId, uint8_t* buf, size_t length) = 0;

    void enable(bool state);
    bool isEnabled() const;

    void configured(bool state);
    bool isConfigured() const;

    virtual int setBaseIndices(unsigned interface, unsigned string, unsigned epMask);
    virtual int getNextAvailableEndpoint(EndpointDirection type, bool reserve = true);
    virtual unsigned getEndpointMask() const;

    virtual int getNumInterfaces() const = 0;
    virtual int getNumStrings() const = 0;

protected:
    Device* dev_ = nullptr;
    bool enabled_ = false;
    unsigned interfaceBase_ = 0;
    unsigned stringBase_ = 0;
    unsigned endpointMask_ = 0;
    volatile bool configured_ = false;
};

} // namespace usbd
} // namespace particle
