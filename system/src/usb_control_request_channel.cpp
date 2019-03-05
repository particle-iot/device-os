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

#include "usb_control_request_channel.h"

#ifdef USB_VENDOR_REQUEST_ENABLE

#include "system_task.h"
#include "system_threading.h"

#include "deviceid_hal.h"

#include "spark_wiring_interrupts.h"

#include "test_malloc.h"
#include "bytes2hexbuf.h"
#include "debug.h"

// FIXME: we should not be polluting our code with such generic macro names
#undef RESET
#undef SET

namespace {

using namespace particle;

// Minimum length of the data stage for high-speed USB devices
const size_t MIN_WLENGTH = 64;

// Service request types as defined by the protocol
enum ServiceRequestType {
    INIT = 1,
    CHECK = 2,
    SEND = 3,
    RECV = 4,
    RESET = 5
};

// Encoder for the service reply data
class ServiceReply {
public:
    // Status codes as defined by the protocol
    enum Status {
        OK = 0,
        ERROR = 1,
        PENDING = 2,
        BUSY = 3,
        NO_MEMORY = 4,
        NOT_FOUND = 5
    };

    ServiceReply() :
            flags_(FieldFlag::STATUS), // `status` is a mandatory field
            result_(SYSTEM_ERROR_NONE),
            size_(0),
            status_(Status::OK),
            id_(USB_REQUEST_INVALID_ID) {
    }

    ServiceReply& status(uint16_t status) {
        status_ = status;
        return *this;
    }

    ServiceReply& id(uint16_t id) {
        id_ = id;
        flags_ |= FieldFlag::ID;
        return *this;
    }

    ServiceReply& size(uint32_t size) {
        size_ = size;
        flags_ |= FieldFlag::SIZE;
        return *this;
    }

    ServiceReply& result(uint32_t result) {
        result_ = result;
        flags_ |= FieldFlag::RESULT;
        return *this;
    }

    bool encode(HAL_USB_SetupRequest* req) const {
        char* d = (char*)req->data;
        if (!d) {
            return false;
        }
        size_t n = req->wLength;
        // Field flags (4 bytes)
        if (!writeUInt32LE(flags_, d, n)) {
            return false;
        }
        // Status code (2 bytes)
        if (!writeUInt16LE(status_, d, n)) {
            return false;
        }
        // Request ID (2 bytes, optional)
        if ((flags_ & FieldFlag::ID) && !writeUInt16LE(id_, d, n)) {
            return false;
        }
        // Payload size (4 bytes, optional)
        if ((flags_ & FieldFlag::SIZE) && !writeUInt32LE(size_, d, n)) {
            return false;
        }
        // Result code (4 bytes, optional)
        if ((flags_ & FieldFlag::RESULT) && !writeUInt32LE(result_, d, n)) {
            return false;
        }
        req->wLength = d - (char*)req->data;
        return true;
    }

private:
    enum FieldFlag {
        STATUS = 0x01,
        ID = 0x02,
        SIZE = 0x04,
        RESULT = 0x08
    };

    uint32_t flags_, result_, size_;
    uint16_t status_, id_;

    static bool writeUInt16LE(uint16_t val, char*& data, size_t& size) {
        if (size < 2) {
            return false;
        }
        *data++ = val & 0xff;
        *data++ = (val >> 8) & 0xff;
        size -= 2;
        return true;
    }

    static bool writeUInt32LE(uint32_t val, char*& data, size_t& size) {
        if (size < 4) {
            return false;
        }
        *data++ = val & 0xff;
        *data++ = (val >> 8) & 0xff;
        *data++ = (val >> 16) & 0xff;
        *data++ = (val >> 24) & 0xff;
        size -= 4;
        return true;
    }
};

inline bool isServiceRequestType(uint8_t bRequest) {
    // Values of the `bRequest` field in the range [0x01, 0x0f] are reserved for service requests
    return (bRequest >= 0x01 && bRequest <= 0x0f);
}

} // namespace

particle::UsbControlRequestChannel::UsbControlRequestChannel(ControlRequestHandler* handler) :
        ControlRequestChannel(handler),
        activeReqs_(nullptr),
        curReq_(nullptr),
        activeReqCount_(0),
        lastReqId_(USB_REQUEST_INVALID_ID) {
    // Set HAL callbacks
    ATOMIC_BLOCK() {
        HAL_USB_Set_Vendor_Request_Callback(halVendorRequestCallback, this);
        HAL_USB_Set_Vendor_Request_State_Callback(halVendorRequestStateCallback, this);
    }
}

particle::UsbControlRequestChannel::~UsbControlRequestChannel() {
    ATOMIC_BLOCK() {
        HAL_USB_Set_Vendor_Request_Callback(nullptr, nullptr);
        HAL_USB_Set_Vendor_Request_State_Callback(nullptr, nullptr);
    }
}

int particle::UsbControlRequestChannel::allocReplyData(ctrl_request* ctrlReq, size_t size) {
    const auto req = static_cast<Request*>(ctrlReq);
    if (size > 0) {
        const auto data = (char*)t_realloc(req->reply_data, size);
        if (!data) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        req->reply_data = data;
    } else {
        t_free(req->reply_data);
        req->reply_data = nullptr;
    }
    req->reply_size = size;
    return SYSTEM_ERROR_NONE;
}

void particle::UsbControlRequestChannel::freeRequestData(ctrl_request* ctrlReq) {
    const auto req = static_cast<Request*>(ctrlReq);
    if (req->flags & RequestFlag::POOLED_REQ_DATA) {
        // Release a pooled buffer
        system_pool_free(req->request_data, nullptr);
        req->flags &= ~RequestFlag::POOLED_REQ_DATA;
    } else {
        // Free a dynamically allocated buffer
        t_free(req->request_data);
    }
    req->request_data = nullptr;
    req->request_size = 0;
}

void particle::UsbControlRequestChannel::setResult(ctrl_request* ctrlReq, int result, ctrl_completion_handler_fn handler,
        void* data) {
    auto req = static_cast<Request*>(ctrlReq);
    if (req->request_data) {
        freeRequestData(req);
    }
    req->handler = handler;
    req->handlerData = data;
    ATOMIC_BLOCK() {
        if (req->state == RequestState::PENDING) {
            req->result = result;
            req->state = RequestState::DONE; // TODO: Start a timer
            req = nullptr;
        }
    }
    if (req) { // Request has been cancelled
        finishRequest(req);
    }
}

// Note: This method is called from an ISR
bool particle::UsbControlRequestChannel::processServiceRequest(HAL_USB_SetupRequest* halReq) {
    switch (halReq->bRequest) { // Service request type
    case ServiceRequestType::INIT:
        return processInitRequest(halReq);
    case ServiceRequestType::CHECK:
        return processCheckRequest(halReq);
    case ServiceRequestType::SEND:
        return processSendRequest(halReq);
    case ServiceRequestType::RECV:
        return processRecvRequest(halReq);
    case ServiceRequestType::RESET:
        return processResetRequest(halReq);
    default:
        return false; // Unknown request type
    }
}

// Note: This method is called from an ISR
bool particle::UsbControlRequestChannel::processInitRequest(HAL_USB_SetupRequest* halReq) {
    if (halReq->wLength < MIN_WLENGTH || !halReq->data) {
        return false; // Unexpected length of the data stage
    }
    if (activeReqCount_ >= USB_REQUEST_MAX_ACTIVE_COUNT) {
        return ServiceReply().status(ServiceReply::BUSY).encode(halReq); // Too many active requests
    }
    // Allocate a request object from the pool
    const auto req = (Request*)system_pool_alloc(sizeof(Request), nullptr);
    if (!req) {
        return ServiceReply().status(ServiceReply::NO_MEMORY).encode(halReq); // Memory allocation error
    }
    req->size = sizeof(ctrl_request);
    req->type = halReq->wIndex; // Request type
    req->request_size = halReq->wValue; // Size of the payload data
    req->reply_data = nullptr;
    req->reply_size = 0;
    req->channel = this;
    req->task.req = req;
    req->handler = nullptr;
    req->handlerData = nullptr;
    req->result = SYSTEM_ERROR_UNKNOWN;
    req->id = ++lastReqId_;
    req->flags = 0;
    // Check if the request has payload data
    ServiceReply::Status status = ServiceReply::ERROR;
    if (req->request_size == 0) {
        // The request has no payload data, invoke the request handler
        req->task.func = invokeRequestHandler;
        SystemISRTaskQueue.enqueue(&req->task);
        req->request_data = nullptr;
        req->state = RequestState::PENDING;
        status = ServiceReply::OK;
    } else if (req->request_size <= USB_REQUEST_MAX_POOLED_BUFFER_SIZE &&
            (req->request_data = (char*)system_pool_alloc(req->request_size, nullptr))) {
        // Device is ready to receive payload data
        req->flags |= RequestFlag::POOLED_REQ_DATA;
        req->state = RequestState::RECV_PAYLOAD; // TODO: Start a timer
        status = ServiceReply::OK;
    } else {
        // The buffer needs to be allocated asynchronously
        req->task.func = allocRequestData;
        SystemISRTaskQueue.enqueue(&req->task);
        req->request_data = nullptr;
        req->state = RequestState::ALLOC_PENDING;
        status = ServiceReply::PENDING;
    }
    // Update list of active requests
    req->prev = nullptr;
    req->next = activeReqs_;
    if (activeReqs_) {
        activeReqs_->prev = req;
    }
    activeReqs_ = req;
    ++activeReqCount_;
    // Reply to the host
    return ServiceReply().id(req->id).status(status).encode(halReq);
}

// Note: This method is called from an ISR
bool particle::UsbControlRequestChannel::processCheckRequest(HAL_USB_SetupRequest* halReq) {
    if (halReq->wLength < MIN_WLENGTH || !halReq->data) {
        return false; // Unexpected length of the data stage
    }
    const uint16_t id = halReq->wIndex; // Request ID
    Request* req = activeReqs_;
    for (; req; req = req->next) {
        if (req->id == id) {
            break;
        }
    }
    ServiceReply rep;
    if (req) {
        switch (req->state) {
        case RequestState::ALLOC_PENDING:
            rep.status(ServiceReply::PENDING);
            break;
        case RequestState::ALLOC_FAILED:
            rep.status(ServiceReply::NO_MEMORY);
            finishActiveRequest(req);
            break;
        case RequestState::RECV_PAYLOAD:
            rep.status(ServiceReply::OK);
            break;
        case RequestState::PENDING:
            rep.status(ServiceReply::PENDING);
            break;
        case RequestState::DONE:
            rep.result(req->result);
            rep.status(ServiceReply::OK);
            if (req->reply_size > 0) {
                rep.size(req->reply_size);
            } else {
                curReq_ = req;
            }
            break;
        }
    } else {
        rep.status(ServiceReply::NOT_FOUND);
    }
    return rep.encode(halReq);
}

// Note: This method is called from an ISR
bool particle::UsbControlRequestChannel::processSendRequest(HAL_USB_SetupRequest* halReq) {
    const uint16_t id = halReq->wIndex; // Request ID
    const uint16_t size = halReq->wLength; // Payload size
    Request* req = activeReqs_;
    for (; req; req = req->next) {
        if (req->id == id) {
            break;
        }
    }
    if (!req || // Request not found
            req->state != RequestState::RECV_PAYLOAD || // Invalid request state
            req->request_size != size) { // Unexpected size
        return false;
    }
    if (size <= MIN_WLENGTH) {
        // Use an internal buffer provided by the HAL
        if (!halReq->data) {
            return false;
        }
        memcpy(req->request_data, halReq->data, size);
    } else if (!halReq->data) {
        // Provide a buffer to the HAL
        halReq->data = (uint8_t*)req->request_data;
        return true;
    }
    // Invoke the request handler
    req->task.func = invokeRequestHandler;
    SystemISRTaskQueue.enqueue(&req->task);
    req->state = RequestState::PENDING;
    return true;
}

// Note: This method is called from an ISR
bool particle::UsbControlRequestChannel::processRecvRequest(HAL_USB_SetupRequest* halReq) {
    const uint16_t id = halReq->wIndex; // Request ID
    const uint16_t size = halReq->wLength; // Payload size
    Request* req = activeReqs_;
    for (; req; req = req->next) {
        if (req->id == id) {
            break;
        }
    }
    if (!req || // Request not found
            req->state != RequestState::DONE || // Invalid request state
            req->reply_size != size || size == 0) { // Unexpected size
        return false;
    }
    if (size <= MIN_WLENGTH) {
        // Use an internal buffer provided by the HAL
        if (!halReq->data) {
            return false;
        }
        memcpy(halReq->data, req->reply_data, size);
    } else {
        // Provide a buffer to the HAL
        halReq->data = (uint8_t*)req->reply_data;
    }
    curReq_ = req;
    return true;
}

// Note: This method is called from an ISR
bool particle::UsbControlRequestChannel::processResetRequest(HAL_USB_SetupRequest* halReq) {
    if (halReq->wLength < MIN_WLENGTH || !halReq->data) {
        return false; // Unexpected length of the data stage
    }
    const uint16_t id = halReq->wIndex; // Request ID
    if (id != USB_REQUEST_INVALID_ID) {
        // Cancel request with a specified ID
        Request* req = activeReqs_;
        for (; req; req = req->next) {
            if (req->id == id) {
                break;
            }
        }
        if (!req) {
            return ServiceReply().status(ServiceReply::NOT_FOUND).encode(halReq);
        }
        // Set a result code that will be passed to the request completion handler
        req->result = SYSTEM_ERROR_CANCELLED;
        finishActiveRequest(req);
    } else {
        // Cancel all requests
        while (activeReqs_) {
            activeReqs_->result = SYSTEM_ERROR_CANCELLED;
            finishActiveRequest(activeReqs_);
        }
    }
    return ServiceReply().status(ServiceReply::OK).encode(halReq);
}

// Note: This method is called from an ISR
bool particle::UsbControlRequestChannel::processVendorRequest(HAL_USB_SetupRequest* req) {
    // In case of a "raw" USB vendor request, the `bRequest` field should be set to the ASCII code
    // of the character 'P' (0x50), and `wIndex` should contain a request type as defined by the
    // `ctrl_request_type` enum
    if (req->bRequest != 0x50) {
        return false;
    }
    if (req->bmRequestTypeDirection != 0) {
        // Device-to-host
        if (!req->wLength || !req->data) {
            return false; // No data stage or requested > 64 bytes
        }
        switch (req->wIndex) {
        case CTRL_REQUEST_DEVICE_ID: {
            // Return the device ID as a hex string
            uint8_t id[16];
            const unsigned n = HAL_device_ID(id, sizeof(id));
            if (n > sizeof(id) || req->wLength < n * 2) {
                return false;
            }
            bytes2hexbuf(id, n, (char*)req->data);
            req->wLength = n * 2;
            break;
        }
        case CTRL_REQUEST_SYSTEM_VERSION: {
            const size_t n = sizeof(PP_STR(SYSTEM_VERSION_STRING)) - 1; // Excluding term. null
            if (req->wLength < n) {
                return false;
            }
            memcpy(req->data, PP_STR(SYSTEM_VERSION_STRING), n);
            req->wLength = n;
            break;
        }
        default:
            return false; // Unknown request type
        }
    } else {
        // Host-to-device
        switch (req->wIndex) {
        default:
            return false; // Unknown request type
        }
    }
    return true;
}

// Note: This method is called from an ISR
void particle::UsbControlRequestChannel::finishActiveRequest(Request* req) {
    // Update list of active requests
    if (req->next) {
        req->next->prev = req->prev;
    }
    if (req->prev) {
        req->prev->next = req->next;
    } else {
        activeReqs_ = req->next;
    }
    --activeReqCount_;
    // Free request data
    if (req->state == RequestState::ALLOC_PENDING || req->state == RequestState::PENDING) { // ALLOC_PENDING, PENDING
        // Mark this request as completed to make the system thread free the request data
        req->state = RequestState::DONE;
    } else { // ALLOC_FAILED, RECV_PAYLOAD, DONE
        if (req->request_data && (req->flags & RequestFlag::POOLED_REQ_DATA)) {
            system_pool_free(req->request_data, nullptr);
            req->request_data = nullptr;
        }
        if (!req->request_data && !req->reply_data && !req->handler) {
            system_pool_free(req, nullptr);
        } else {
            // Free the request data asynchronously
            req->task.func = finishRequest;
            SystemISRTaskQueue.enqueue(&req->task);
        }
    }
}

void particle::UsbControlRequestChannel::finishRequest(Request* req) {
    if (req->request_data) {
        freeRequestData(req);
    }
    if (req->reply_data) {
        allocReplyData(req, 0);
    }
    if (req->handler) {
        req->handler(req->result, req->handlerData);
    }
    system_pool_free(req, nullptr);
}

// Note: This method is called from an ISR
void particle::UsbControlRequestChannel::invokeRequestHandler(ISRTaskQueue::Task* isrTask) {
    const auto task = static_cast<RequestTask*>(isrTask);
    const auto req = task->req;
    const auto channel = static_cast<UsbControlRequestChannel*>(req->channel);
    channel->handler()->processRequest(req, channel);
}

// Note: This method is called from an ISR
void particle::UsbControlRequestChannel::allocRequestData(ISRTaskQueue::Task* isrTask) {
    const auto task = static_cast<RequestTask*>(isrTask);
    auto req = task->req;
    req->request_data = (char*)t_malloc(req->request_size); // FIXME: volatile?
    ATOMIC_BLOCK() {
        if (req->state == RequestState::ALLOC_PENDING) {
            if (req->request_data) {
                req->state = RequestState::RECV_PAYLOAD; // TODO: Start a timer
            } else {
                req->state = RequestState::ALLOC_FAILED;
            }
            req = nullptr;
        }
    }
    if (req) { // Request has been cancelled
        t_free(req->request_data);
        system_pool_free(req, nullptr);
    }
}

// Note: This method is called from an ISR
void particle::UsbControlRequestChannel::finishRequest(ISRTaskQueue::Task* isrTask) {
    const auto task = static_cast<RequestTask*>(isrTask);
    const auto req = task->req;
    const auto channel = static_cast<UsbControlRequestChannel*>(req->channel);
    channel->finishRequest(req);
}

/*
    This callback should process vendor-specific SETUP requests from the host.
    NOTE: This callback is called from an ISR.

    Each request contains the following fields:
    - bmRequestType - request type bit mask. Since only vendor-specific device requests are forwarded
                      to this callback, the only bit that should be of intereset is
                      "Data Phase Transfer Direction", easily accessed as req->bmRequestTypeDirection:
                      0 - Host to Device
                      1 - Device to Host
    - bRequest - 1 byte, request identifier. The only reserved request that will not be forwarded to this callback
                 is 0xee, which is used for Microsoft-specific vendor requests.
    - wIndex - 2 byte index, any value between 0x0000 and 0xffff.
    - wValue - 2 byte value, any value between 0x0000 and 0xffff.
    - wLength - each request might have an optional data stage of up to 0xffff (65535) bytes.
                Host -> Device requests contain data sent by the host to the device.
                Device -> Host requests request the device to send up to wLength bytes of data.

    This callback should return 0 if the request has been correctly handled or 1 otherwise.

    When handling Device->Host requests with data stage, this callback should fill req->data buffer with
    up to wLength bytes of data if req->data != NULL (which should be the case when wLength <= 64)
    or set req->data to point to some buffer which contains up to wLength bytes of data.
    wLength may be safely modified to notify that there is less data in the buffer than requested by the host.

    [1] Host -> Device requests with data stage containing up to 64 bytes can be handled by
    an internal buffer in HAL. For requests larger than 64 bytes, the vendor request callback
    needs to provide a buffer of an appropriate size:

    req->data = buffer; // sizeof(buffer) >= req->wLength
    return 0;

    The callback will be called again once the data stage completes
    It will contain the same bmRequest, bRequest, wIndex, wValue and wLength fields.
    req->data should contain wLength bytes received from the host.
*/
uint8_t particle::UsbControlRequestChannel::halVendorRequestCallback(HAL_USB_SetupRequest* halReq, void* data) {
    const auto channel = static_cast<UsbControlRequestChannel*>(data);
    bool ok = false;
    if (isServiceRequestType(halReq->bRequest)) {
        ok = channel->processServiceRequest(halReq);
    } else {
        ok = channel->processVendorRequest(halReq);
    }
    return (ok ? 0 : 1);
}

// Note: This method is called from an ISR
uint8_t particle::UsbControlRequestChannel::halVendorRequestStateCallback(HAL_USB_VendorRequestState state, void* data) {
    const auto channel = static_cast<UsbControlRequestChannel*>(data);
    switch (state) {
    case HAL_USB_VENDOR_REQUEST_STATE_TX_COMPLETED: {
        if (channel->curReq_) {
            // Set a result code that will be passed to the request completion handler
            channel->curReq_->result = SYSTEM_ERROR_NONE;
            channel->finishActiveRequest(channel->curReq_);
            channel->curReq_ = nullptr;
        }
        break;
    }
    case HAL_USB_VENDOR_REQUEST_STATE_RESET: {
        // Cancel all requests
        while (channel->activeReqs_) {
            channel->activeReqs_->result = SYSTEM_ERROR_ABORTED;
            channel->finishActiveRequest(channel->activeReqs_);
        }
        channel->curReq_ = nullptr;
        break;
    }
    default:
        break;
    }
    return 0;
}

#endif // defined(USB_VENDOR_REQUEST_ENABLE)
