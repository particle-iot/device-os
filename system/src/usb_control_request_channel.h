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
#include "concurrent_hal.h"

#include "system_error.h"

#include <memory>

#ifdef USB_VENDOR_REQUEST_ENABLE

namespace particle {

// Size of the request object pool. This parameter limits the maximum number of asynchronous
// requests that can be processed concurrently
const size_t USB_REQUEST_POOL_SIZE = 2;

// Settings of the request buffer pool
const size_t USB_REQUEST_PREALLOC_BUFFER_SIZE = 256;
const size_t USB_REQUEST_PREALLOC_BUFFER_COUNT = 2;

// Maximum size of the payload data
const size_t USB_REQUEST_MAX_PAYLOAD_SIZE = 65535;

// Time given to the host to start sending request data or receiving reply data
const unsigned USB_REQUEST_HOST_TIMEOUT = 1000;

// Invalid request ID
const uint16_t USB_REQUEST_INVALID_ID = 0;

static_assert(USB_REQUEST_POOL_SIZE > 0, "The request object pool cannot be empty");
static_assert(USB_REQUEST_PREALLOC_BUFFER_SIZE > 0, "Size of a preallocated buffer should be greater than zero");

// Class implementing the protocol for asynchronous USB requests
class UsbControlRequestChannel: public ControlRequestChannel {
public:
    explicit UsbControlRequestChannel(ControlRequestHandler* handler);
    ~UsbControlRequestChannel();

    // ControlRequestChannel
    virtual void freeRequestData(ctrl_request* ctrlReq) override;
    virtual int allocReplyData(ctrl_request* ctrlReq, size_t size) override;
    virtual void setResult(ctrl_request* ctrlReq, system_error_t result) override;

private:
    // Request state
    enum State {
        ALLOC_PENDING, // Buffer allocation is pending
        ALLOC_FAILED, // Buffer allocation failed
        WAIT_DATA, // Waiting for the host to send payload data
        PROCESSING, // Request is being processed by the handler
        DONE // Request processing is completed
    };

    // Preallocated buffer
    struct Buffer {
        char data[USB_REQUEST_PREALLOC_BUFFER_SIZE]; // Buffer data
        Buffer* next; // Next entry in the pool
    };

    // Request data
    struct Request {
        ctrl_request ctrl; // Control request data (should be the first field in this structure)
        UsbControlRequestChannel* channel; // Request channel
        State state; // Request state
        system_tick_t time; // State timestamp
        Buffer* reqBuf; // Preallocated request buffer
        Buffer* repBuf; // Preallocated reply buffer
        Request* next; // Next entry in the pool
        int result; // Result code
        uint16_t id; // Request ID
    };

    std::unique_ptr<Request[]> reqs_; // Request pool
    std::unique_ptr<Buffer[]> bufs_; // Buffer pool
    os_timer_t timer_;

    Request* activeReqList_; // List of active requests
    Request* freeReqList_; // List of unused request objects
    Buffer* freeBufList_; // List of unused preallocated buffers
    uint16_t lastReqId_; // Last request ID

    bool processServiceRequest(HAL_USB_SetupRequest* halReq);
    bool processVendorRequest(HAL_USB_SetupRequest* req);

    static void timeout(os_timer_t timer);

    static void invokeRequestHandler(void* data);
    static void allocRequestBuffer(void* data);
    static void freeReplyData(void* data);

    static uint8_t halVendorRequestCallback(HAL_USB_SetupRequest* req, void* data);
};

} // namespace particle

#endif // defined(USB_VENDOR_REQUEST_ENABLE)
