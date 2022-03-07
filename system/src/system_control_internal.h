/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
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

#include "system_control.h"

#if SYSTEM_CONTROL_ENABLED

#include "control_request_handler.h"
#include "usb_control_request_channel.h"
#include "ble_control_request_channel.h"
#include "spark_wiring_vector.h"

#include "debug.h"

using spark::Vector;

namespace particle {

namespace system {

class SystemControl: public ControlRequestHandler {
public:
    SystemControl();

    int setAppRequestHandler(ctrl_request_handler_fn handler);

    int allocReplyData(ctrl_request* req, size_t size);
    void freeReplyData(ctrl_request* req);
    void freeRequestData(ctrl_request* req);
    void setResult(ctrl_request* req, int result, ctrl_completion_handler_fn handler = nullptr, void* data = nullptr);

    // TODO: Use a separate thread for the BLE channel loop
    int init();
    void run();

    // ControlRequestHandler
    virtual void processRequest(ctrl_request* req, ControlRequestChannel* channel) override;

    static SystemControl* instance();

    system_ctrl_acl getDefaultFilterAction();
    system_ctrl_filter* getFiltersList();
    void setDefaultFilterAction(system_ctrl_acl act);
    void setFiltersList(system_ctrl_filter* filters);

#if HAL_PLATFORM_BLE
    BleControlRequestChannel* getBleCtrlRequestChannel();
#endif

private:
#ifdef USB_VENDOR_REQUEST_ENABLE
    UsbControlRequestChannel usbChannel_;
#endif
#if HAL_PLATFORM_BLE_SETUP
    BleControlRequestChannel bleChannel_;
#endif
    ctrl_request_handler_fn appReqHandler_;

    void processAppRequest(ctrl_request* req);

    system_ctrl_filter* filters_;
    system_ctrl_acl defaultFilterAction_;
};

inline void SystemControl::setDefaultFilterAction(system_ctrl_acl act) {
    defaultFilterAction_ = act;
}

inline system_ctrl_acl SystemControl::getDefaultFilterAction() {
    return defaultFilterAction_;
}

inline void SystemControl::setFiltersList(system_ctrl_filter* filters) {
    // Clear/free() the existing filters_
    system_ctrl_filter* tmp;
    while (filters_ != nullptr)
    {
        tmp = filters_;
        filters_ = filters_->next;
        free(tmp);
    }
    filters_ = filters;
}

inline system_ctrl_filter* SystemControl::getFiltersList() {
    return filters_;
}

inline int SystemControl::setAppRequestHandler(ctrl_request_handler_fn handler) {
    appReqHandler_ = handler;
    return 0;
}

inline int SystemControl::allocReplyData(ctrl_request* req, size_t size) {
    const auto channel = static_cast<ControlRequestChannel*>(req->channel);
    return channel->allocReplyData(req, size);
}

inline void SystemControl::freeReplyData(ctrl_request* req) {
    SPARK_ASSERT(allocReplyData(req, 0) == 0);
}

inline void SystemControl::freeRequestData(ctrl_request* req) {
    const auto channel = static_cast<ControlRequestChannel*>(req->channel);
    channel->freeRequestData(req);
}

inline void SystemControl::setResult(ctrl_request* req, int result, ctrl_completion_handler_fn handler,
        void* data) {
    const auto channel = static_cast<ControlRequestChannel*>(req->channel);
    channel->setResult(req, result, handler, data);
}

#if HAL_PLATFORM_BLE
inline BleControlRequestChannel* SystemControl::getBleCtrlRequestChannel() {
    return &bleChannel_;
}
#endif

} // particle::system

} // particle

#endif // SYSTEM_CONTROL_ENABLED
