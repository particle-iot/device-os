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
#include "control_request_handler.h"
#include "usb_hal.h"

#include "system_error.h"

#include <memory>

#ifdef USB_VENDOR_REQUEST_ENABLE

namespace particle {

// Settings of the request buffer pool
const size_t USB_REQUEST_PREALLOC_BUFFER_SIZE = 256;
const size_t USB_REQUEST_PREALLOC_BUFFER_COUNT = 2;

// Size of the request object pool. This parameter limits the maximum number of asynchronous
// requests that can be processed concurrently
const size_t USB_REQUEST_POOL_SIZE = 2;

// Timeout for the host to start sending request data or receiving reply data
const system_tick_t USB_REQUEST_HOST_TIMEOUT = 1000;

// Maximum size of the payload data
const size_t USB_REQUEST_MAX_PAYLOAD_SIZE = 65535;

// Invalid request ID
const uint16_t USB_REQUEST_INVALID_ID = 0;

static_assert(USB_REQUEST_POOL_SIZE > 0, "The request object pool cannot be empty");

// Class implementing the protocol for asynchronous USB requests.
class UsbControlRequestChannel: public ControlRequestChannel {
public:
    explicit UsbControlRequestChannel(ControlRequestHandler* handler);
    ~UsbControlRequestChannel();

    // ControlRequestChannel
    virtual int allocReplyData(ctrl_request* req, size_t size) override;
    virtual void freeReplyData(ctrl_request* req) override;
    virtual void freeRequestData(ctrl_request* req) override;
    virtual void setResult(ctrl_request* req, system_error_t result) override;

private:
    // Request state
    enum State {
        ALLOC, // Buffer allocation is pending
        RECV, // Receiving payload data
        PENDING, // Request processing is in progress
        DONE // Request processing is completed
    };

    // Preallocated buffer
    struct Buffer {
        char data[USB_REQUEST_PREALLOC_BUFFER_SIZE];
        Buffer* next; // Next entry in the pool
    };

    // Request data
    struct Request {
        ctrl_request ctrl; // Control request data (should be the first field in this structure)
        ControlRequestHandler* handler; // Request handler
        State state; // Request state
        system_tick_t time; // State timestamp
        Buffer* reqBuf; // Request buffer
        Buffer* repBuf; // Reply buffer
        Request* next; // Next entry in the pool
        int result; // Result code
        uint16_t id; // Request ID
    };

    std::unique_ptr<Request[]> reqs_;
    std::unique_ptr<Buffer[]> bufs_;
    Request* activeReq_; // List of active requests
    Request* freeReq_; // List of unused requests
    Buffer* freeBuf_; // List of unused buffers
    unsigned readyReqCount_;
    uint16_t lastReqId_;

    bool processServiceRequest(HAL_USB_SetupRequest* halReq);
    bool processVendorRequest(HAL_USB_SetupRequest* req);

    static void processRequest(void* data);
    static void allocRequestBuffer(void* data);

    static uint8_t halVendorRequestCallback(HAL_USB_SetupRequest* req, void* data);
};

} // namespace particle

#endif // defined(USB_VENDOR_REQUEST_ENABLE)
