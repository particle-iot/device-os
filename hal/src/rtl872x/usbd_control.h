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

#include "usbd_device.h"
#include "usbd_wcid.h"
#include "hal_platform.h"
#include "usb_hal.h"

namespace particle { namespace usbd {

namespace detail {

} /* namespace detail */

class ControlInterfaceClassDriver : public particle::usbd::ClassDriver {
public:
    static ControlInterfaceClassDriver* instance();
    ~ControlInterfaceClassDriver();

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
    virtual unsigned getEndpointMask() const override;

    void setVendorRequestCallback(HAL_USB_Vendor_Request_Callback cb, void* ptr);
    void setVendorRequestStateCallback(HAL_USB_Vendor_Request_State_Callback cb, void* p);

protected:
    ControlInterfaceClassDriver();

private:
    int handleMsftRequest(particle::usbd::SetupRequest* req);
    int handleVendorRequest(particle::usbd::SetupRequest* req);

    HAL_USB_Vendor_Request_Callback requestCallback_ = nullptr;
    void* requestCallbackCtx_ = nullptr;
    HAL_USB_Vendor_Request_State_Callback stateCallback_ = nullptr;
    void* stateCallbackCtx_ = nullptr;

    HAL_USB_SetupRequest request_ = {};
    uint8_t requestData_[EP0_MAX_PACKET_SIZE] = {};

    enum class State {
        NONE = 0,
        TX = 1,
        RX = 2
    };
    State state_ = State::NONE;
    volatile uint32_t counter_ = 0;
};

} } /* namespace particle::usbd */

