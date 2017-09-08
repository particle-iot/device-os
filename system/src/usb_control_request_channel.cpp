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

#include "system_threading.h"

#include "timer_hal.h"
#include "deviceid_hal.h"

#include "spark_wiring_interrupts.h"

#include "bytes2hexbuf.h"
#include "debug.h"

LOG_SOURCE_CATEGORY("system.ctrl.usb");

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

    ServiceReply& status(Status status) {
        status_ = (uint16_t)status;
        return *this;
    }

    ServiceReply& id(uint16_t id) {
        id_ = id;
        flags_ |= FieldFlag::ID;
        return *this;
    }

    ServiceReply& size(size_t size) {
        size_ = size;
        flags_ |= FieldFlag::SIZE;
        return *this;
    }

    ServiceReply& result(system_error_t result) {
        result_ = (uint32_t)result;
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
    // Values of the `bRequest` field in the range [0x01, 0x0f] are reserved for the service requests
    return (bRequest >= 0x01 && bRequest <= 0x0f);
}

inline system_tick_t now() {
    return HAL_Timer_Get_Milli_Seconds();
}

} // namespace

particle::UsbControlRequestChannel::UsbControlRequestChannel(ControlRequestHandler* handler) :
        ControlRequestChannel(handler),
        timer_(OS_TIMER_INVALID_HANDLE),
        activeReqList_(nullptr),
        freeReqList_(nullptr),
        freeBufList_(nullptr),
        lastReqId_(USB_REQUEST_INVALID_ID) {
    // Initialize timer
    if (os_timer_create(&timer_, USB_REQUEST_HOST_TIMEOUT, timeout, this, true /* one_shot */, nullptr /* reserved */) != 0) {
        LOG(ERROR, "Unable to initialize a timer");
        return;
    }
    // Initialize request object pool
    reqs_.reset(new(std::nothrow) Request[USB_REQUEST_POOL_SIZE]);
    if (!reqs_) {
        LOG(ERROR, "Unable to allocate memory for the request object pool");
        return;
    }
    for (size_t i = 0; i < USB_REQUEST_POOL_SIZE; ++i) {
        Request* const req = reqs_.get() + i;
        req->ctrl.size = sizeof(ctrl_request);
        req->channel = this;
        req->next = (i != USB_REQUEST_POOL_SIZE - 1) ? req + 1 : nullptr;
    }
    freeReqList_ = reqs_.get();
    // Initialize request buffer pool
    if (USB_REQUEST_PREALLOC_BUFFER_COUNT > 0) {
        bufs_.reset(new(std::nothrow) Buffer[USB_REQUEST_PREALLOC_BUFFER_COUNT]);
        if (bufs_) {
            for (size_t i = 0; i < USB_REQUEST_PREALLOC_BUFFER_COUNT; ++i) {
                Buffer* const buf = bufs_.get() + i;
                buf->next = (i != USB_REQUEST_PREALLOC_BUFFER_COUNT - 1) ? buf + 1 : nullptr;
            }
            freeBufList_ = bufs_.get();
        } else {
            LOG(WARN, "Unable to allocate memory for the request buffer pool");
        }
    }
    HAL_USB_Set_Vendor_Request_Callback(halVendorRequestCallback, this);
}

particle::UsbControlRequestChannel::~UsbControlRequestChannel() {
    if (reqs_) {
        HAL_USB_Set_Vendor_Request_Callback(nullptr, nullptr);
    }
}

void particle::UsbControlRequestChannel::freeRequestData(ctrl_request* ctrlReq) {
    const auto req = (Request*)ctrlReq;
    char* data = nullptr;
    ATOMIC_BLOCK() {
        // Check if the request uses a preallocated buffer
        if (req->reqBuf) {
            req->reqBuf->next = freeBufList_;
            freeBufList_ = req->reqBuf;
            req->reqBuf = nullptr;
        } else {
            data = req->ctrl.request_data;
        }
        req->ctrl.request_data = nullptr;
        req->ctrl.request_size = 0;
    }
    // Free a dynamically allocated buffer
    delete[] data;
}

int particle::UsbControlRequestChannel::allocReplyData(ctrl_request* ctrlReq, size_t size) {
    const auto req = (Request*)ctrlReq;
    // Try to use a preallocated buffer
    Buffer* buf = nullptr;
    if (size <= USB_REQUEST_PREALLOC_BUFFER_SIZE) {
        ATOMIC_BLOCK() {
            buf = freeBufList_;
            if (buf) {
                freeBufList_ = buf->next;
                req->ctrl.reply_data = buf->data;
                req->ctrl.reply_size = size;
                req->repBuf = buf;
            }
        }
    }
    if (!buf) {
        // Allocate the buffer dynamically
        char* data = new(std::nothrow) char[size];
        if (!data) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        ATOMIC_BLOCK() {
            req->ctrl.reply_data = data;
            req->ctrl.reply_size = size;
        }
    }
    return SYSTEM_ERROR_NONE;
}

void particle::UsbControlRequestChannel::setResult(ctrl_request* ctrlReq, system_error_t result) {
    freeRequestData(ctrlReq);
    const auto req = (Request*)ctrlReq;
    ATOMIC_BLOCK() {
        req->result = result;
        req->time = now();
        req->state = State::DONE;
    }
}

bool particle::UsbControlRequestChannel::processServiceRequest(HAL_USB_SetupRequest* halReq) {
    switch (halReq->bRequest) {
    // INIT
    case ServiceRequestType::INIT: {
        if (halReq->wLength < MIN_WLENGTH || !halReq->data) {
            return false; // Unexpected length of the data stage
        }
        // Check if a request object is available
        Request* const req = freeReqList_;
        if (!req) {
            return ServiceReply().status(ServiceReply::BUSY).encode(halReq); // Device is busy
        }
        ServiceReply::Status status = ServiceReply::ERROR;
        // Check if a buffer needs to be allocated for the payload data
        req->reqBuf = nullptr;
        req->ctrl.request_size = halReq->wValue; // Size of the payload data
        if (req->ctrl.request_size == 0) {
            if (!SystemISRTaskQueue.enqueue(invokeRequestHandler, req)) {
                return ServiceReply().status(ServiceReply::BUSY).encode(halReq); // Device is busy
            }
            req->state = State::PROCESSING; // The request has no payload data
            req->ctrl.request_data = nullptr;
            status = ServiceReply::OK;
        } else {
            if (req->ctrl.request_size <= USB_REQUEST_PREALLOC_BUFFER_SIZE) {
                req->reqBuf = freeBufList_;
            }
            if (req->reqBuf) {
                req->state = State::WAIT_DATA; // Device is ready to receive payload data
                req->ctrl.request_data = req->reqBuf->data;
                status = ServiceReply::OK;
            } else {
                if (!SystemISRTaskQueue.enqueue(allocRequestBuffer, req)) {
                    return ServiceReply().status(ServiceReply::BUSY).encode(halReq); // Device is busy
                }
                req->state = State::ALLOC_PENDING; // Request buffer needs to be allocated dynamically
                req->ctrl.request_data = nullptr;
                status = ServiceReply::PENDING;
            }
        }
        // Initialize remaining request data
        req->ctrl.reply_data = nullptr;
        req->ctrl.reply_size = 0;
        req->ctrl.type = halReq->wIndex;
        req->repBuf = nullptr;
        req->time = now();
        req->result = SYSTEM_ERROR_NONE;
        req->id = ++lastReqId_;
        // Update pools
        freeReqList_ = req->next;
        if (req->reqBuf) {
            freeBufList_ = req->reqBuf->next;
        }
        // Update list of active requests
        req->next = activeReqList_;
        activeReqList_ = req;
        // Reply to the host
        return ServiceReply().id(req->id).status(status).encode(halReq);
    }
    // CHECK
    case ServiceRequestType::CHECK: {
        if (halReq->wLength < MIN_WLENGTH || !halReq->data) {
            return false; // Unexpected length of the data stage
        }
        const uint16_t id = halReq->wIndex; // Request ID
        Request* req = activeReqList_;
        while (req) {
            if (req->id == id) {
                break;
            }
            req = req->next;
        }
        ServiceReply svcRep;
        if (req) {
            switch (req->state) {
            case State::ALLOC_PENDING:
                svcRep.status(ServiceReply::PENDING);
                break;
            case State::ALLOC_FAILED:
                svcRep.status(ServiceReply::NO_MEMORY);
                break;
            case State::WAIT_DATA:
                svcRep.status(ServiceReply::OK);
                break;
            case State::PROCESSING:
                svcRep.status(ServiceReply::PENDING);
                break;
            case State::DONE:
                svcRep.size(req->ctrl.reply_size);
                svcRep.result((system_error_t)req->result);
                svcRep.status(ServiceReply::OK);
                break;
            }
        } else {
            svcRep.status(ServiceReply::NOT_FOUND);
        }
        return svcRep.encode(halReq);
    }
    // SEND
    case ServiceRequestType::SEND: {
        const uint16_t id = halReq->wIndex; // Request ID
        const uint16_t size = halReq->wLength; // Payload size
        Request* req = activeReqList_;
        while (req) {
            if (req->id == id) {
                break;
            }
            req = req->next;
        }
        if (!req || // Request not found
                req->state != State::WAIT_DATA || // Invalid request state
                req->ctrl.request_size != size) { // Unexpected size
            return false;
        }
        if (size <= MIN_WLENGTH) {
            // Use an internal buffer provided by the HAL
            if (!halReq->data) {
                return false;
            }
            memcpy(req->ctrl.request_data, halReq->data, size);
        } else if (!halReq->data) {
            // Provide a buffer to the HAL
            halReq->data = (uint8_t*)req->ctrl.request_data;
            return true;
        }
        if (!SystemISRTaskQueue.enqueue(invokeRequestHandler, req)) {
            return false; // Device is busy
        }
        req->time = now();
        req->state = State::PROCESSING;
        return true;
    }
    // RECV
    case ServiceRequestType::RECV: {
        const uint16_t id = halReq->wIndex; // Request ID
        const uint16_t size = halReq->wLength; // Payload size
        Request* req = activeReqList_;
        while (req) {
            if (req->id == id) {
                break;
            }
            req = req->next;
        }
        if (!req || // Request not found
                req->state != State::DONE || // Invalid request state
                req->ctrl.reply_size != size) { // Unexpected size
            return false;
        }
        if (size <= MIN_WLENGTH) {
            // Use an internal buffer provided by the HAL
            if (!halReq->data) {
                return false;
            }
            memcpy(halReq->data, req->ctrl.reply_data, size);
        } else {
            // Provide a buffer to the HAL
            halReq->data = (uint8_t*)req->ctrl.reply_data;
        }
        return true;
    }
    // RESET
    case ServiceRequestType::RESET: {
        if (halReq->wLength < MIN_WLENGTH || !halReq->data) {
            return false; // Unexpected length of the data stage
        }
        // TODO: We cannot really cancel requests which are being processed in the system thread,
        // since with the fixed pool of request objects there is always a risk of a race condition
        return ServiceReply().status(ServiceReply::OK).encode(halReq);
    }
    default:
        return false; // Unknown request type
    }
}

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
            const size_t n = sizeof(PP_STR(SYSTEM_VERSION_STRING));
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

void particle::UsbControlRequestChannel::invokeRequestHandler(void* data) {
    const auto req = static_cast<Request*>(data);
    ControlRequestHandler* const handler = req->channel->handler();
    handler->processRequest((ctrl_request*)req, req->channel);
}

void particle::UsbControlRequestChannel::allocRequestBuffer(void* data) {
    const auto req = static_cast<Request*>(data);
    const auto d = new(std::nothrow) char[req->ctrl.request_size];
    ATOMIC_BLOCK() {
        if (d) {
            req->ctrl.request_data = d;
            req->state = State::WAIT_DATA;
        } else {
            req->state = State::ALLOC_FAILED;
        }
    }
}

void particle::UsbControlRequestChannel::freeReplyData(void* data) {
    delete[] static_cast<char*>(data);
}

void particle::UsbControlRequestChannel::timeout(os_timer_t timer) {
    // TODO
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
uint8_t particle::UsbControlRequestChannel::halVendorRequestCallback(HAL_USB_SetupRequest* req, void* data) {
    const auto d = static_cast<UsbControlRequestChannel*>(data);
    bool ok = false;
    if (d) {
        if (isServiceRequestType(req->bRequest)) {
            ok = d->processServiceRequest(req);
        } else {
            ok = d->processVendorRequest(req);
        }
    }
    return (ok ? 0 : 1);
}
