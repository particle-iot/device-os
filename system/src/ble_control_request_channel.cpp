/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "logging.h"

LOG_SOURCE_CATEGORY("system.ctrl.ble")

#include "ble_control_request_channel.h"

#if SYSTEM_CONTROL_ENABLED && HAL_PLATFORM_BLE

#include "device_code.h"

#include "preprocessor.h"

#ifndef PAIRING_ENABLED
#define PAIRING_ENABLED 0
#endif

namespace particle {

namespace system {

namespace {

// Particle's company ID
const unsigned COMPANY_ID = 0x0662;

// Device setup protocol version
const unsigned PROTOCOL_VERSION = 0x01;

// Vendor-specific base UUID: 6FA9xxxx-5C4E-48A8-94F4-8030546F36FC
const uint8_t BASE_UUID[16] = { 0xfc, 0x36, 0x6f, 0x54, 0x30, 0x80, 0xf4, 0x94, 0xa8, 0x48, 0x4e, 0x5c, 0x00, 0x00, 0xa9, 0x6f };

// UUID of the control request service
const unsigned CTRL_SERVICE_UUID = 0x0001;

// UUID of the protocol version characteristic
const unsigned VERSION_CHAR_UUID = 0x0002;

// UUID of the characteristic used to send reply data
const unsigned SEND_CHAR_UUID = 0x0003;

// UUID of the characteristic used to receive request data
const unsigned RECV_CHAR_UUID = 0x0004;

// Size of the buffer for reply data
const unsigned SEND_BUF_SIZE = BLE_MAX_ATTR_VALUE_SIZE;

// Size of the buffer for request data
const unsigned RECV_BUF_SIZE = 1024; // Should be a power of two

static_assert(SEND_BUF_SIZE >= BLE_MAX_ATTR_VALUE_SIZE && RECV_BUF_SIZE >= BLE_MAX_ATTR_VALUE_SIZE,
        "Invalid buffer size");

struct __attribute__((packed)) RequestHeader {
    uint16_t id;
    uint16_t type;
    uint32_t size;
};

struct __attribute__((packed)) ReplyHeader {
    uint16_t id;
    int16_t result;
    uint32_t size;
};

} // particle::system::

BleControlRequestChannel::BleControlRequestChannel(ControlRequestHandler* handler) :
        ControlRequestChannel(handler),
        pendingReqs_(nullptr),
        readyReqs_(nullptr),
        sendReq_(nullptr),
        sendPos_(0),
        headerSent_(false),
        recvReq_(nullptr),
        recvFifo_(),
        recvPos_(0),
        headerRecvd_(false),
        sendCharHandle_(BLE_INVALID_ATTR_HANDLE),
        recvCharHandle_(BLE_INVALID_ATTR_HANDLE),
        connHandle_(BLE_INVALID_CONN_HANDLE),
        maxCharValSize_(0),
        notifEnabled_(false),
        writable_(false) {
}

BleControlRequestChannel::~BleControlRequestChannel() {
    destroy();
}

int BleControlRequestChannel::init() {
    uint32_t nrfRet = 0;
    int ret = initProfile();
    if (ret != 0) {
        goto error;
    }
    // TODO: Allocate necessary buffers when a BLE connection is accepted
    sendBuf_.reset(new(std::nothrow) uint8_t[SEND_BUF_SIZE]);
    if (!sendBuf_) {
        ret = SYSTEM_ERROR_NO_MEMORY;
        goto error;
    }
    recvBuf_.reset(new(std::nothrow) uint8_t[RECV_BUF_SIZE]);
    if (!recvBuf_) {
        ret = SYSTEM_ERROR_NO_MEMORY;
        goto error;
    }
    nrfRet = app_fifo_init(&recvFifo_, recvBuf_.get(), RECV_BUF_SIZE);
    if (nrfRet != NRF_SUCCESS) {
        LOG(ERROR, "app_fifo_init() failed: %u", (unsigned)nrfRet);
        ret = SYSTEM_ERROR_UNKNOWN;
        goto error;
    }
    return 0;
error:
    destroy();
    return ret;
}

void BleControlRequestChannel::destroy() {
    // TODO: There doesn't seem to be a straightforward way to uninitialize the profile
    sendBuf_.reset();
    recvBuf_.reset();
}

int BleControlRequestChannel::run() {
    if (connHandle_ == BLE_INVALID_CONN_HANDLE) {
        // Free completed requests
        while (readyReqs_) {
            readyReqs_ = freeRequest(readyReqs_);
        }
        freeRequest(sendReq_);
        sendReq_ = nullptr;
        freeRequest(recvReq_);
        recvReq_ = nullptr;
        // Clear buffers and reset state
        app_fifo_flush(&recvFifo_);
        sendPos_ = 0;
        recvPos_ = 0;
        headerSent_ = false;
        headerRecvd_ = false;
    } else {
        // Proceed sending and receiving data
        int ret = receiveNext();
        if (ret == 0) {
            ret = sendNext();
        }
        if (ret != 0) {
            LOG(ERROR, "Connection error");
            ble_disconnect(connHandle_, nullptr);
            connHandle_ = BLE_INVALID_CONN_HANDLE;
            return ret;
        }
    }
    return 0;
}

void BleControlRequestChannel::setResult(ctrl_request* ctrlReq, int result, ctrl_completion_handler_fn handler, void* data) {
    // FIXME: Synchronization
    // TODO: Completion handling
    const auto req = static_cast<Request*>(ctrlReq);
    req->result = result;
    req->next = readyReqs_;
    readyReqs_ = req;
}

int BleControlRequestChannel::sendNext() {
    if (!writable_) {
        return 0; // Can't send now
    }
    if (!sendReq_) {
        if (!readyReqs_) {
            return 0; // Nothing to send
        }
        sendReq_ = readyReqs_;
        readyReqs_ = readyReqs_->next;
    }
    // Serialize reply header
    uint16_t size = 0;
    if (!headerSent_) {
        ReplyHeader h = {};
        h.id = sendReq_->id;
        h.result = sendReq_->result;
        h.size = sendReq_->reply_size;
        memcpy(sendBuf_.get(), &h, sizeof(ReplyHeader));
        size = sizeof(ReplyHeader);
    }
    // Copy reply data
    size_t dataSize = 0;
    if (sendReq_->reply_size > 0) {
        dataSize = sendReq_->reply_size - sendPos_;
        if (size + dataSize > maxCharValSize_) {
            dataSize = maxCharValSize_ - size;
        }
        memcpy(sendBuf_.get() + size, sendReq_->reply_data + sendPos_, dataSize);
        size += dataSize;
    }
    // Send packet
    const int ret = ble_set_char_value(connHandle_, sendCharHandle_, (const char*)sendBuf_.get(), size,
            BLE_SET_CHAR_VALUE_FLAG_NOTIFY, nullptr);
    if (ret == BLE_ERROR_BUSY) {
        writable_ = false; // Retry later
        return 0;
    }
    if (ret != size) {
        LOG(ERROR, "ble_set_char_value() failed: %d", ret);
        return ret;
    }
    LOG_DEBUG(TRACE, "%u bytes sent", (unsigned)size);
    sendPos_ += dataSize;
    if (sendPos_ == sendReq_->reply_size) {
        freeRequest(sendReq_);
        sendReq_ = nullptr;
        sendPos_ = 0;
        headerSent_ = false;
    } else {
        headerSent_ = true;
    }
    return 0;
}

int BleControlRequestChannel::receiveNext() {
    // Read request header
    if (!headerRecvd_) {
        uint32_t n = 0;
        const auto ret = app_fifo_read(&recvFifo_, nullptr, &n);
        if (ret != NRF_SUCCESS && ret != NRF_ERROR_NOT_FOUND) {
            LOG(ERROR, "app_fifo_read() failed: %u", (unsigned)ret);
            return SYSTEM_ERROR_UNKNOWN;
        }
        if (n < sizeof(RequestHeader)) {
            return 0; // Keep reading
        }
        RequestHeader h = {};
        n = sizeof(RequestHeader);
        app_fifo_read(&recvFifo_, (uint8_t*)&h, &n);
        recvReq_ = allocRequest(h.size);
        if (!recvReq_) {
            LOG(ERROR, "Unable to allocate request object");
            return SYSTEM_ERROR_NO_MEMORY;
        }
        recvReq_->id = h.id;
        recvReq_->type = h.type;
        recvReq_->channel = this;
        headerRecvd_ = true;
    }
    // Read request data
    if (recvReq_->request_size > 0) {
        uint32_t n = recvReq_->request_size - recvPos_;
        const auto ret = app_fifo_read(&recvFifo_, (uint8_t*)recvReq_->request_data + recvPos_, &n);
        if (ret != NRF_SUCCESS && ret != NRF_ERROR_NOT_FOUND) {
            LOG(ERROR, "app_fifo_read() failed: %u", (unsigned)ret);
            return SYSTEM_ERROR_UNKNOWN;
        }
        recvPos_ += n;
        if (recvPos_ < recvReq_->request_size) {
            return 0; // Keep reading
        }
    }
    // Process request
    recvReq_->next = pendingReqs_;
    pendingReqs_ = recvReq_;
    recvReq_ = nullptr;
    recvPos_ = 0;
    headerRecvd_ = false;
    handler()->processRequest(pendingReqs_, this);
    return 0;
}

void BleControlRequestChannel::connected(const ble_connected_event_data& event) {
    connHandle_ = event.conn_handle;
    // Get initial connection parameters
    ble_conn_param connParam = { .version = BLE_API_VERSION };
    int ret = ble_get_conn_param(connHandle_, &connParam, nullptr);
    maxCharValSize_ = (ret == 0) ? connParam.max_char_value_size : BLE_MAX_ATTR_VALUE_SIZE;
    ble_char_param charParam = { .version = BLE_API_VERSION };
    ret = ble_get_char_param(connHandle_, sendCharHandle_, &charParam, nullptr);
    notifEnabled_ = (ret == 0) ? charParam.notif_enabled : false;
    writable_ = notifEnabled_;
}

void BleControlRequestChannel::disconnected(const ble_disconnected_event_data& event) {
    connHandle_ = BLE_INVALID_CONN_HANDLE;
    maxCharValSize_ = 0;
    notifEnabled_ = false;
    writable_ = false;
}

void BleControlRequestChannel::connParamChanged(const ble_conn_param_changed_event_data& event) {
    ble_conn_param param = { .version = BLE_API_VERSION };
    const int ret = ble_get_conn_param(connHandle_, &param, nullptr);
    if (ret == 0) {
        maxCharValSize_ = param.max_char_value_size;
    } else {
        LOG(ERROR, "Unable to get connection parameters");
        ble_disconnect(connHandle_, nullptr);
        connHandle_ = BLE_INVALID_CONN_HANDLE;
    }
}

void BleControlRequestChannel::charParamChanged(const ble_char_param_changed_event_data& event) {
    if (event.char_handle == sendCharHandle_) {
        ble_char_param param = { .version = BLE_API_VERSION };
        const int ret = ble_get_char_param(connHandle_, sendCharHandle_, &param, nullptr);
        if (ret == 0) {
            notifEnabled_ = param.notif_enabled;
            writable_ = notifEnabled_;
        } else {
            LOG(ERROR, "Unable to get characteristic parameters");
            ble_disconnect(connHandle_, nullptr);
            connHandle_ = BLE_INVALID_CONN_HANDLE;
        }
    }
}

void BleControlRequestChannel::dataSent(const ble_data_sent_event_data& event) {
    if (notifEnabled_) {
        writable_ = true;
    }
}

void BleControlRequestChannel::dataReceived(const ble_data_received_event_data& event) {
    if (event.char_handle == recvCharHandle_) {
        uint32_t size = event.size;
        const auto ret = app_fifo_write(&recvFifo_, (const uint8_t*)event.data, &size);
        if (ret != NRF_SUCCESS) {
            LOG(ERROR, "app_fifo_write() failed: %u", (unsigned)ret);
            ble_disconnect(connHandle_, nullptr);
            connHandle_ = BLE_INVALID_CONN_HANDLE;
        }
        if (size != event.size) {
            LOG(ERROR, "Incomplete write, increase buffer size");
            ble_disconnect(connHandle_, nullptr);
            connHandle_ = BLE_INVALID_CONN_HANDLE;
        }
        LOG_DEBUG(TRACE, "%u bytes received", (unsigned)event.size);
    }
}

int BleControlRequestChannel::initProfile() {
    // Register base UUID
    uint8_t uuidType = 0;
    int ret = ble_add_base_uuid((const char*)BASE_UUID, &uuidType, nullptr);
    if (ret != 0) {
        return ret;
    }
    // Characteristics
    ble_char chars[3] = {};
    // Protocol version
    ble_char& verChar = chars[0];
    verChar.uuid.type = uuidType;
    verChar.uuid.uuid = VERSION_CHAR_UUID;
    verChar.type = BLE_CHAR_TYPE_VAL;
    const char charVal = PROTOCOL_VERSION;
    verChar.data = &charVal;
    verChar.size = sizeof(charVal);
    // TX
    ble_char& sendChar = chars[1];
    sendChar.uuid.type = uuidType;
    sendChar.uuid.uuid = SEND_CHAR_UUID;
    sendChar.type = BLE_CHAR_TYPE_TX;
#if PAIRING_ENABLED
    sendChar.flags = BLE_CHAR_FLAG_REQUIRE_PAIRING;
#endif
    // RX
    ble_char& recvChar = chars[2];
    recvChar.uuid.type = uuidType;
    recvChar.uuid.uuid = RECV_CHAR_UUID;
    recvChar.type = BLE_CHAR_TYPE_RX;
#if PAIRING_ENABLED
    recvChar.flags = BLE_CHAR_FLAG_REQUIRE_PAIRING;
#endif
    // Services
    ble_service ctrlService = {};
    ctrlService.uuid.type = uuidType;
    ctrlService.uuid.uuid = CTRL_SERVICE_UUID;
    ctrlService.chars = chars;
    ctrlService.char_count = sizeof(chars) / sizeof(chars[0]);
    // Manufacturer-specific data
    const uint16_t platformId = PLATFORM_ID;
    ble_manuf_data manufData = {};
    manufData.company_id = COMPANY_ID;
    manufData.data = (const char*)&platformId;
    manufData.size = sizeof(platformId);
    // Initialize profile
    ble_profile profile = {};
    profile.version = BLE_API_VERSION;
    char devName[32] = {};
    ret = get_device_name(devName, sizeof(devName));
    if (ret < 0) {
        LOG(ERROR, "Unable to get device name");
        return ret;
    }
    profile.device_name = devName;
    profile.services = &ctrlService;
    profile.service_count = 1;
#if PAIRING_ENABLED
    profile.flags = BLE_PROFILE_FLAG_ENABLE_PAIRING;
#endif
    profile.manuf_data = &manufData;
    profile.callback = processBleEvent;
    profile.user_data = this;
    ret = ble_init_profile(&profile, nullptr);
    if (ret != 0) {
        return ret;
    }
    sendCharHandle_ = sendChar.handle;
    recvCharHandle_ = recvChar.handle;
    return 0;
}

void BleControlRequestChannel::processBleEvent(int event, const void* eventData, void* userData) {
    const auto that = (BleControlRequestChannel*)userData;
    switch (event) {
    case BLE_EVENT_CONNECTED: {
        const auto d = (const ble_connected_event_data*)eventData;
        that->connected(*d);
        break;
    }
    case BLE_EVENT_DISCONNECTED: {
        const auto d = (const ble_disconnected_event_data*)eventData;
        that->disconnected(*d);
        break;
    }
    case BLE_EVENT_CONN_PARAM_CHANGED: {
        const auto d = (const ble_conn_param_changed_event_data*)eventData;
        that->connParamChanged(*d);
        break;
    }
    case BLE_EVENT_CHAR_PARAM_CHANGED: {
        const auto d = (const ble_char_param_changed_event_data*)eventData;
        that->charParamChanged(*d);
        break;
    }
    case BLE_EVENT_DATA_SENT: {
        const auto d = (const ble_data_sent_event_data*)eventData;
        that->dataSent(*d);
        break;
    }
    case BLE_EVENT_DATA_RECEIVED: {
        const auto d = (const ble_data_received_event_data*)eventData;
        that->dataReceived(*d);
        break;
    }
    default:
        break;
    }
}

BleControlRequestChannel::Request* BleControlRequestChannel::allocRequest(size_t size) {
    const auto req = (Request*)malloc(sizeof(Request));
    if (!req) {
        return nullptr;
    }
    memset(req, 0, sizeof(Request));
    if (size > 0) {
        req->request_data = (char*)malloc(size);
        if (!req->request_data) {
            free(req);
            return nullptr;
        }
        req->request_size = size;
    }
    return req;
}

BleControlRequestChannel::Request* BleControlRequestChannel::freeRequest(Request* req) {
    if (!req) {
        return nullptr;
    }
    const auto next = req->next;
    free(req->request_data);
    free(req->reply_data);
    free(req);
    return next;
}

} // particle::system

} // particle

#endif // SYSTEM_CONTROL_ENABLED && HAL_PLATFORM_BLE
