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

#pragma once

#include "usbd_device.h"
#include "usbd_wcid.h"
#include "hal_platform.h"
#include "usb_hal.h"
#include <functional>

namespace particle { namespace usbd {

namespace hid {

enum Request {
    SET_PROTOCOL = 0x0B,
    GET_PROTOCOL = 0x03,
    SET_IDLE = 0x0A,
    GET_IDLE = 0x02,
    SET_REPORT = 0x09,
    GET_REPORT = 0x01,
    INVALID_REQUEST = 0xff
};

enum DescriptorType {
    DESCRIPTOR_HID = 0x21,
    DESCRIPTOR_HID_REPORT = 0x22
};

const size_t MAX_DATA_PACKET_SIZE = 64;

} /* namespace hid */

class HidClassDriver : public particle::usbd::ClassDriver {
public:
    static HidClassDriver* instance();
    ~HidClassDriver();

    virtual int init(unsigned cfgIdx) override;
    virtual int deinit(unsigned cfgIdx) override;
    virtual int setup(particle::usbd::SetupRequest* req) override;
    virtual int startOfFrame() override;
    virtual int dataIn(unsigned ep, particle::usbd::EndpointEvent ev, size_t len) override;
    virtual int dataOut(unsigned ep, particle::usbd::EndpointEvent ev, size_t len) override;
    virtual int getConfigurationDescriptor(uint8_t* buf, size_t length, Speed speed = Speed::FULL, unsigned index = 0) override;
    virtual int getString(unsigned id, uint16_t langId, uint8_t* buf, size_t length) override;

    virtual int getNumInterfaces() const override;
    virtual int getNumStrings() const override;

    void setName(const char* name);

    int sendReport(const uint8_t* report, size_t size);
    int transferStatus() const;
    int setDigitizerState(bool state);

private:
    int handleInSetupRequest(SetupRequest *req);
    int handleOutSetupRequest(SetupRequest *req);
    size_t reportDescriptorLen() const;

protected:
    HidClassDriver();

private:
    char* name_ = nullptr;
    uint8_t epIn_ = ENDPOINT_INVALID;

    volatile bool txState_ = false;

    __attribute__((aligned(4))) uint8_t buffer_[hid::MAX_DATA_PACKET_SIZE] = {};

    uint8_t altSetting_ = 0;
    uint8_t protocol_ = 0;
    uint8_t idleState_ = 0;
    bool digitizerState_ = true;
};

} } /* namespace particle::usbd */

