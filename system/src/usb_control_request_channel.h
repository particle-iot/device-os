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

#include "usb_hal.h"

#ifdef USB_VENDOR_REQUEST_ENABLE

#include "system_control.h"
#include "control_request_handler.h"
#include "active_object.h"

namespace particle {

// Maximum number of asynchronous requests that can be active at the same time
const size_t USB_REQUEST_MAX_ACTIVE_COUNT = 4;

// Maximum size of the payload data
const size_t USB_REQUEST_MAX_PAYLOAD_SIZE = 65535;

// Maximum size of a request buffer that can be allocated from the memory pool
const size_t USB_REQUEST_MAX_POOLED_BUFFER_SIZE = 64;

// Invalid request ID
const uint16_t USB_REQUEST_INVALID_ID = 0;

// Class implementing the asynchronous USB request protocol
class UsbControlRequestChannel: public ControlRequestChannel {
public:
    explicit UsbControlRequestChannel(ControlRequestHandler* handler);
    ~UsbControlRequestChannel();

    // ControlRequestChannel
    virtual int allocReplyData(ctrl_request* ctrlReq, size_t size) override;
    virtual void freeRequestData(ctrl_request* ctrlReq) override;
    virtual void setResult(ctrl_request* req, int result, ctrl_completion_handler_fn handler, void* data) override;

private:
    // Request state
    enum RequestState {
        ALLOC_PENDING, // Buffer allocation is pending
        ALLOC_FAILED, // Buffer allocation failed
        RECV_PAYLOAD, // Waiting for the host to send payload data
        PENDING, // Request is being processed by the handler
        DONE // Request processing is completed
    };

    // Request flags
    enum RequestFlag {
        POOLED_REQ_DATA = 0x01 // Request buffer is allocated from the pool
    };

    struct Request;

    // ISR task data
    struct RequestTask: ISRTaskQueue::Task {
        Request* req;
    };

    // Request data
    struct Request: ctrl_request {
        RequestTask task; // ISR task data
        Request* prev; // Previous element in a list
        Request* next; // Next element in a list
        ctrl_completion_handler_fn handler; // Completion handler
        void* handlerData; // Completion handler data
        int result; // Result code
        uint16_t id; // Request ID
        uint8_t state; // Request state
        uint8_t flags; // Request flags
    };

    Request* activeReqs_; // List of active requests
    Request* curReq_; // A request currently being processed by the USB subsystem
    uint16_t activeReqCount_; // Number of active requests
    uint16_t lastReqId_; // Last request ID

    bool processServiceRequest(HAL_USB_SetupRequest* halReq);
    bool processInitRequest(HAL_USB_SetupRequest* halReq);
    bool processCheckRequest(HAL_USB_SetupRequest* halReq);
    bool processSendRequest(HAL_USB_SetupRequest* halReq);
    bool processRecvRequest(HAL_USB_SetupRequest* halReq);
    bool processResetRequest(HAL_USB_SetupRequest* halReq);
    bool processVendorRequest(HAL_USB_SetupRequest* halReq);

    void finishActiveRequest(Request* req);
    void finishRequest(Request* req);

    static void invokeRequestHandler(ISRTaskQueue::Task* isrTask);
    static void allocRequestData(ISRTaskQueue::Task* isrTask);
    static void finishRequest(ISRTaskQueue::Task* isrTask);

    static uint8_t halVendorRequestCallback(HAL_USB_SetupRequest* halReq, void* data);
    static uint8_t halVendorRequestStateCallback(HAL_USB_VendorRequestState state, void* data);
};

} // namespace particle

#endif // defined(USB_VENDOR_REQUEST_ENABLE)
