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

namespace particle {

class ControlRequestChannel;

// Base abstract class for a control request handler
class ControlRequestHandler {
public:
    virtual void processRequest(ctrl_request* req, ControlRequestChannel* channel) = 0;
};

// Base abstract class for a control request channel
class ControlRequestChannel {
public:
    explicit ControlRequestChannel(ControlRequestHandler* handler);

    virtual int allocReplyData(ctrl_request* req, size_t size);
    virtual void freeRequestData(ctrl_request* req);
    virtual void setResult(ctrl_request* req, int result, ctrl_completion_handler_fn handler = nullptr, void* data = nullptr) = 0;

protected:
    ControlRequestHandler* handler() const;

private:
    ControlRequestHandler* handler_;
};

} // namespace particle

inline particle::ControlRequestChannel::ControlRequestChannel(ControlRequestHandler* handler) :
        handler_(handler) {
}

inline particle::ControlRequestHandler* particle::ControlRequestChannel::handler() const {
    return handler_;
}
