/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#ifndef USBD_DEVICE_H
#define USBD_DEVICE_H

#include <cstddef>
#include <cstdint>

namespace particle { namespace usbd {

/* Forward declaration */
class ClassDriver;

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
    TYPE_CLASS    = 0x01,
    TYPE_VENDOR   = 0x02
  };

  enum Recipient {
    RECIPIENT_DEVICE    = 0x00,
    RECIPIENT_INTERFACE = 0x01,
    RECIPIENT_ENDPOINT  = 0x02,
    RECIPIENT_OTHER     = 0x03
  };

  enum StandardRequest {
    REQUEST_GET_STATUS        = 0x00,
    REQUEST_CLEAR_FEATURE     = 0x01,
    REQUEST_SET_FEATURE       = 0x03,
    REQUEST_SET_ADDRESS       = 0x05,
    REQUEST_GET_DESCRIPTOR    = 0x06,
    REQUEST_SET_DESCRIPTOR    = 0x07,
    REQUEST_GET_CONFIGURATION = 0x08,
    REQUEST_SET_CONFIGURATION = 0x09,
    REQUEST_GET_INTERFACE     = 0x0a,
    REQUEST_SET_INTERFACE     = 0x0b,
    REQUEST_SYNCH_FRAME       = 0x0c
  };
};

class Device {
public:
  Device() = default;
  Device(const Device&) = delete;

  virtual int registerClass(ClassDriver* drv) = 0;
  virtual int openEndpoint(unsigned ep, size_t pakSize, unsigned type) = 0;
  virtual int closeEndpoint(unsigned ep) = 0;
  virtual int flushEndpoint(unsigned ep) = 0;
  virtual int transferIn(unsigned ep, const uint8_t* ptr, size_t size) = 0;
  virtual int transferOut(unsigned ep, uint8_t* ptr, size_t size) = 0;

  virtual void setupReply(SetupRequest* r, const uint8_t* data, size_t size) = 0;
  virtual void setupReceive(SetupRequest* r, uint8_t* data, size_t size) = 0;
  virtual void setupError(SetupRequest* r) = 0;

  virtual void detach() = 0;
  virtual void attach() = 0;

  enum Status {
    STATUS_DEFAULT = 0,
    STATUS_ADDRESSED,
    STATUS_CONFIGURED,
    STATUS_SUSPENDED
  };

  enum DescriptorType {
    DESCRIPTOR_DEVICE                    = 0x01,
    DESCRIPTOR_CONFIGURATION             = 0x02,
    DESCRIPTOR_STRING                    = 0x03,
    DESCRIPTOR_INTERFACE                 = 0x04,
    DESCRIPTOR_ENDPOINT                  = 0x05,
    DESCRIPTOR_DEVICE_QUALIFIER          = 0x06,
    DESCRIPTOR_OTHER_SPEED_CONFIGURATION = 0x07,
    DESCRIPTOR_OTG                       = 0x09
  };

  enum DefaultString {
    STRING_IDX_LANGID       = 0x00,
    STRING_IDX_MANUFACTURER = 0x01,
    STRING_IDX_PRODUCT      = 0x02,
    STRING_IDX_SERIAL       = 0x03,
    STRING_IDX_CONFIG       = 0x04,
    STRING_IDX_INTERFACE    = 0x05,
    STRING_IDX_MSFT         = 0xee
  };

  enum Feature {
    FEATURE_EP_HALT = 0x00,
    FEATURE_DEVICE_REMOTE_WAKEUP = 0x01
  };

  void setDeviceStatus(Status st) {
    s_ = st;
  }

  Status getDeviceStatus() const {
    return s_;
  }

private:
  Status s_ = STATUS_DEFAULT;
};

class ClassDriver {
public:
  virtual int init(unsigned cfgIdx) = 0;
  virtual int deInit(unsigned cfgIdx) = 0;
  virtual int setup(SetupRequest* req) = 0;
  virtual int dataIn(uint8_t ep, void* ex) = 0;
  virtual int dataOut(uint8_t ep, void* ex) = 0;
  virtual int startOfFrame() = 0;
  virtual int outDone(uint8_t ep, unsigned status) = 0;
  virtual int inDone(uint8_t ep, unsigned status) = 0;
  virtual const uint8_t* getConfigurationDescriptor(uint16_t* length) = 0;
  virtual const uint8_t* getStringDescriptor(unsigned index, uint16_t* length, bool* conv) = 0;
};

} } /* namespace particle::usbd */

#endif /* USBD_DEVICE_H */