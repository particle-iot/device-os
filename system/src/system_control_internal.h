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

#include "debug.h"

namespace particle {

class SystemControl: public ControlRequestHandler {
public:
    SystemControl();

    int setAppRequestHandler(ctrl_request_handler_fn handler);

    int allocReplyData(ctrl_request* req, size_t size);
    void freeReplyData(ctrl_request* req);
    void freeRequestData(ctrl_request* req);
    void setResult(ctrl_request* req, int result, ctrl_completion_handler_fn handler = nullptr, void* data = nullptr);

    // ControlRequestHandler
    virtual void processRequest(ctrl_request* req, ControlRequestChannel* channel) override;

    static SystemControl* instance();

private:
    UsbControlRequestChannel usbReqChannel_;
    ctrl_request_handler_fn appReqHandler_;

    void processAppRequest(ctrl_request* req);
};

} // namespace particle

inline int particle::SystemControl::setAppRequestHandler(ctrl_request_handler_fn handler) {
    appReqHandler_ = handler;
    return 0;
}

inline int particle::SystemControl::allocReplyData(ctrl_request* req, size_t size) {
    const auto channel = static_cast<ControlRequestChannel*>(req->channel);
    return channel->allocReplyData(req, size);
}

inline void particle::SystemControl::freeReplyData(ctrl_request* req) {
    SPARK_ASSERT(allocReplyData(req, 0) == 0);
}

inline void particle::SystemControl::freeRequestData(ctrl_request* req) {
    const auto channel = static_cast<ControlRequestChannel*>(req->channel);
    channel->freeRequestData(req);
}

inline void particle::SystemControl::setResult(ctrl_request* req, int result, ctrl_completion_handler_fn handler,
        void* data) {
    const auto channel = static_cast<ControlRequestChannel*>(req->channel);
    channel->setResult(req, result, handler, data);
}

#endif // SYSTEM_CONTROL_ENABLED
