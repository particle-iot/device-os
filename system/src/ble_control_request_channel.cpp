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
#include "deviceid_hal.h"

#include "endian.h"

#ifndef PAIRING_ENABLED
#define PAIRING_ENABLED 0
#endif

namespace particle {

namespace system {

namespace {

// Message type
enum class MessageType: uint8_t {
    REQUEST = 0x01,
    REPLY = 0x02
};

// Header containing fields which are common for request and reply messages. These fields are only
// authenticated but not encrypted
struct __attribute__((packed)) MessageHeader {
    uint16_t size; // Payload size
};

// Request message header
struct __attribute__((packed)) RequestHeader {
    uint16_t id; // Request ID
    uint16_t type; // Request type
    uint16_t reserved;
};

// Reply message header
struct __attribute__((packed)) ReplyHeader {
    uint16_t id; // Request ID
    int32_t result; // Result code
};

// Particle's company ID
const unsigned COMPANY_ID = 0x0662;

// Device setup protocol version
const unsigned PROTOCOL_VERSION = 0x02;

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

// Size of the buffer for request data
const size_t RECV_BUF_SIZE = 1024; // Should be a power of two

// Size of the message header in bytes
const size_t MESSAGE_HEADER_SIZE = sizeof(MessageHeader);

// Size of the request header in bytes
const size_t REQUEST_HEADER_SIZE = sizeof(RequestHeader);

// Size of the reply header in bytes
const size_t REPLY_HEADER_SIZE = sizeof(ReplyHeader);

// Size of the cipher's key in bytes
const size_t AES_CCM_KEY_SIZE = 16;

// Size of the cipher's block in bytes
const size_t AES_CCM_BLOCK_SIZE = 16;

// Size of the authentication field in bytes
const size_t AES_CCM_TAG_SIZE = 8; // M

// Size of the length field in bytes
const size_t AES_CCM_LENGTH_SIZE = 4; // L

// Size of the nonce in bytes
const size_t AES_CCM_NONCE_SIZE = 11; // 15 - L

void genNonce(uint8_t* nonce, uint32_t counter, MessageType msgType) {
    memset(nonce, 0, 6); // TODO: RFC 3610 suggests using an endpoint's address as part of the nonce
    nonce[6] = (counter >> 0) & 0xff;
    nonce[7] = (counter >> 8) & 0xff;
    nonce[8] = (counter >> 16) & 0xff;
    nonce[9] = (counter >> 24) & 0xff;
    nonce[10] = (uint8_t)msgType;
}

} // particle::system::

BleControlRequestChannel::BleControlRequestChannel(ControlRequestHandler* handler) :
        ControlRequestChannel(handler),
        readyReqs_(nullptr),
        sendReq_(nullptr),
        sendOffs_(0),
        recvFifo_(),
        recvReq_(nullptr),
        recvOffs_(0),
        aesCcm_(),
        reqCount_(0),
        repCount_(0),
        sendCharHandle_(BLE_INVALID_ATTR_HANDLE),
        recvCharHandle_(BLE_INVALID_ATTR_HANDLE),
        connHandle_(BLE_INVALID_CONN_HANDLE),
        halConnHandle_(BLE_INVALID_CONN_HANDLE),
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
    // TODO: Allocate this buffer when a BLE connection is accepted
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
    recvBuf_.reset();
}

void BleControlRequestChannel::run() {
    int ret = 0;
    const auto halConnHandle = halConnHandle_;
    // TODO: Use an event queue for communication between the ISR and the channel loop
    if (connHandle_ != halConnHandle) {
        // Free completed requests
        while (readyReqs_) {
            readyReqs_ = freeRequest(readyReqs_);
        }
        // Clear buffers and reset channel state
        freeRequest(sendReq_);
        sendReq_ = nullptr;
        sendOffs_ = 0;
        app_fifo_flush(&recvFifo_);
        freeRequest(recvReq_);
        recvReq_ = nullptr;
        recvOffs_ = 0;
        reqCount_ = 0;
        repCount_ = 0;
        if (connHandle_ == BLE_INVALID_CONN_HANDLE) { // Connected
            ret = initAesCcm();
        } else if (halConnHandle == BLE_INVALID_CONN_HANDLE) { // Disconnected
            mbedtls_ccm_free(&aesCcm_);
        }
        connHandle_ = halConnHandle;
        if (ret != 0) {
            goto error;
        }
    }
    if (connHandle_ != BLE_INVALID_CONN_HANDLE) {
        // Proceed sending and receiving data
        ret = receiveNext();
        if (ret != 0) {
            goto error;
        }
        ret = sendNext();
        if (ret != 0) {
            goto error;
        }
    }
    return;
error:
    LOG(ERROR, "Channel error: %d", ret);
    if (connHandle_ != BLE_INVALID_CONN_HANDLE) {
        ble_disconnect(connHandle_, nullptr);
    }
}

int BleControlRequestChannel::allocReplyData(ctrl_request* ctrlReq, size_t size) {
    const auto req = static_cast<Request*>(ctrlReq);
    const auto p = (char*)realloc(req->repBuf, size + MESSAGE_HEADER_SIZE + REPLY_HEADER_SIZE + AES_CCM_TAG_SIZE);
    if (!p) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    req->repBuf = p;
    req->reply_data = (size > 0) ? req->repBuf + MESSAGE_HEADER_SIZE + REPLY_HEADER_SIZE : nullptr;
    req->reply_size = size;
    return 0;
}

void BleControlRequestChannel::freeRequestData(ctrl_request* ctrlReq) {
    const auto req = static_cast<Request*>(ctrlReq);
    free(req->reqBuf);
    req->reqBuf = nullptr;
    req->request_data = nullptr;
    req->request_size = 0;
}

void BleControlRequestChannel::setResult(ctrl_request* ctrlReq, int result, ctrl_completion_handler_fn handler, void* data) {
    // FIXME: Synchronization
    // TODO: Completion handling
    const auto req = static_cast<Request*>(ctrlReq);
    freeRequestData(req);
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
        // Make sure we have a buffer for serialized reply data
        if (!sendReq_->repBuf) {
            const int ret = allocReplyData(sendReq_, 0);
            if (ret != 0) {
                return ret;
            }
        }
        // Encrypt the message
        const int ret = encodeReply(sendReq_);
        if (ret != 0) {
            return ret;
        }
    }
    // Send BLE packet
    size_t size = sendReq_->reply_size + MESSAGE_HEADER_SIZE + REPLY_HEADER_SIZE + AES_CCM_TAG_SIZE - sendOffs_;
    if (size > maxCharValSize_) {
        size = maxCharValSize_;
    }
    const int ret = ble_set_char_value(connHandle_, sendCharHandle_, sendReq_->repBuf + sendOffs_, size,
            BLE_SET_CHAR_VALUE_FLAG_NOTIFY, nullptr);
    if (ret == BLE_ERROR_BUSY) {
        writable_ = false; // Retry later
        return 0;
    }
    if (ret != (int)size) {
        LOG(ERROR, "ble_set_char_value() failed: %d", ret);
        return ret;
    }
    LOG_DEBUG(TRACE, "%u bytes sent", (unsigned)size);
    sendOffs_ += size;
    if (sendOffs_ == sendReq_->reply_size + MESSAGE_HEADER_SIZE + REPLY_HEADER_SIZE + AES_CCM_TAG_SIZE) {
        freeRequest(sendReq_);
        sendReq_ = nullptr;
        sendOffs_ = 0;
    }
    return 0;
}

int BleControlRequestChannel::receiveNext() {
    if (!recvReq_) {
        // Read message header
        uint32_t n = 0;
        const auto ret = app_fifo_read(&recvFifo_, nullptr, &n);
        if (ret != NRF_SUCCESS && ret != NRF_ERROR_NOT_FOUND) {
            LOG(ERROR, "app_fifo_read() failed: %u", (unsigned)ret);
            return SYSTEM_ERROR_UNKNOWN;
        }
        if (n < MESSAGE_HEADER_SIZE) {
            return 0; // Keep reading
        }
        MessageHeader h = {};
        n = MESSAGE_HEADER_SIZE;
        app_fifo_read(&recvFifo_, (uint8_t*)&h, &n);
        recvReq_ = allocRequest(littleEndianToNative(h.size));
        if (!recvReq_) {
            LOG(ERROR, "Unable to allocate request object");
            return SYSTEM_ERROR_NO_MEMORY;
        }
        memcpy(recvReq_->reqBuf, &h, MESSAGE_HEADER_SIZE);
        recvOffs_ = MESSAGE_HEADER_SIZE;
    }
    // Read message data
    const size_t reqSize = recvReq_->request_size + MESSAGE_HEADER_SIZE + REQUEST_HEADER_SIZE + AES_CCM_TAG_SIZE;
    uint32_t n = reqSize - recvOffs_;
    const auto nrfRet = app_fifo_read(&recvFifo_, (uint8_t*)recvReq_->reqBuf + recvOffs_, &n);
    if (nrfRet != NRF_SUCCESS && nrfRet != NRF_ERROR_NOT_FOUND) {
        LOG(ERROR, "app_fifo_read() failed: %u", (unsigned)nrfRet);
        return SYSTEM_ERROR_UNKNOWN;
    }
    recvOffs_ += n;
    if (recvOffs_ < reqSize) {
        return 0; // Keep reading
    }
    // Decrypt and process the request
    const int ret = parseRequest(recvReq_);
    if (ret != 0) {
        return ret;
    }
    handler()->processRequest(recvReq_, this);
    recvReq_ = nullptr;
    recvOffs_ = 0;
    return 0;
}

int BleControlRequestChannel::parseRequest(Request* req) {
    const auto p = (uint8_t*)req->reqBuf;
    // Decrypt message
    uint8_t nonce[AES_CCM_NONCE_SIZE] = {};
    genNonce(nonce, ++reqCount_, MessageType::REQUEST);
    const int ret = mbedtls_ccm_auth_decrypt(&aesCcm_,
            req->request_size + REQUEST_HEADER_SIZE, // Size of the input data
            nonce, AES_CCM_NONCE_SIZE, // Nonce
            p, MESSAGE_HEADER_SIZE, // Additional data
            p + MESSAGE_HEADER_SIZE, // Input buffer
            p + MESSAGE_HEADER_SIZE, // Output buffer
            p + req->request_size + MESSAGE_HEADER_SIZE + REQUEST_HEADER_SIZE, AES_CCM_TAG_SIZE); // Authentication tag
    if (ret != 0) {
        LOG(ERROR, "mbedtls_ccm_auth_decrypt() failed: %d", ret);
        return SYSTEM_ERROR_BAD_DATA;
    }
    // Parse request header
    RequestHeader rh = {};
    memcpy(&rh, p + MESSAGE_HEADER_SIZE, REQUEST_HEADER_SIZE);
    rh.id = littleEndianToNative(rh.id); // Request ID
    rh.type = littleEndianToNative(rh.type); // Request type
    req->id = rh.id;
    req->type = rh.type;
    return 0;
}

int BleControlRequestChannel::encodeReply(Request* req) {
    const auto p = (uint8_t*)req->reqBuf;
    // Serialize message header
    MessageHeader mh = {};
    mh.size = nativeToLittleEndian(req->reply_size);
    memcpy(p, &mh, MESSAGE_HEADER_SIZE);
    // Serialize reply header
    ReplyHeader rh = {};
    rh.id = nativeToLittleEndian(req->id);
    rh.result = nativeToLittleEndian(req->result);
    memcpy(p + MESSAGE_HEADER_SIZE, &rh, REPLY_HEADER_SIZE);
    // Encrypt message
    uint8_t nonce[AES_CCM_NONCE_SIZE] = {};
    genNonce(nonce, ++repCount_, MessageType::REPLY);
    const int ret = mbedtls_ccm_encrypt_and_tag(&aesCcm_,
            req->reply_size + REPLY_HEADER_SIZE, // Size of the input data
            nonce, AES_CCM_NONCE_SIZE, // Nonce
            p, MESSAGE_HEADER_SIZE, // Additional data
            p + MESSAGE_HEADER_SIZE, // Input buffer
            p + MESSAGE_HEADER_SIZE, // Output buffer
            p + req->reply_size + MESSAGE_HEADER_SIZE + REPLY_HEADER_SIZE, AES_CCM_TAG_SIZE); // Authentication tag
    if (ret != 0) {
        LOG(ERROR, "mbedtls_ccm_encrypt_and_tag() failed: %d", ret);
        return SYSTEM_ERROR_UNKNOWN;
    }
    return 0;
}

void BleControlRequestChannel::connected(const ble_connected_event_data& event) {
    halConnHandle_ = event.conn_handle;
    // Get initial connection parameters
    ble_conn_param connParam = { .version = BLE_API_VERSION };
    int ret = ble_get_conn_param(halConnHandle_, &connParam, nullptr);
    maxCharValSize_ = (ret == 0) ? connParam.max_char_value_size : BLE_MAX_ATTR_VALUE_SIZE;
    ble_char_param charParam = { .version = BLE_API_VERSION };
    ret = ble_get_char_param(halConnHandle_, sendCharHandle_, &charParam, nullptr);
    notifEnabled_ = (ret == 0) ? charParam.notif_enabled : false;
    writable_ = notifEnabled_;
}

void BleControlRequestChannel::disconnected(const ble_disconnected_event_data& event) {
    halConnHandle_ = BLE_INVALID_CONN_HANDLE;
    maxCharValSize_ = 0;
    notifEnabled_ = false;
    writable_ = false;
}

void BleControlRequestChannel::connParamChanged(const ble_conn_param_changed_event_data& event) {
    ble_conn_param param = { .version = BLE_API_VERSION };
    const int ret = ble_get_conn_param(halConnHandle_, &param, nullptr);
    if (ret == 0) {
        maxCharValSize_ = param.max_char_value_size;
    } else {
        LOG(ERROR, "Unable to get connection parameters");
        ble_disconnect(halConnHandle_, nullptr);
        halConnHandle_ = BLE_INVALID_CONN_HANDLE;
    }
}

void BleControlRequestChannel::charParamChanged(const ble_char_param_changed_event_data& event) {
    if (event.char_handle == sendCharHandle_) {
        ble_char_param param = { .version = BLE_API_VERSION };
        const int ret = ble_get_char_param(halConnHandle_, sendCharHandle_, &param, nullptr);
        if (ret == 0) {
            notifEnabled_ = param.notif_enabled;
            writable_ = notifEnabled_;
        } else {
            LOG(ERROR, "Unable to get characteristic parameters");
            ble_disconnect(halConnHandle_, nullptr);
            halConnHandle_ = BLE_INVALID_CONN_HANDLE;
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
        LOG_DEBUG(TRACE, "%u bytes received", (unsigned)event.size);
        uint32_t size = event.size;
        const auto ret = app_fifo_write(&recvFifo_, (const uint8_t*)event.data, &size);
        if (ret != NRF_SUCCESS || size != event.size) {
            LOG(ERROR, "app_fifo_write() failed");
            ble_disconnect(halConnHandle_, nullptr);
            halConnHandle_ = BLE_INVALID_CONN_HANDLE;
        }
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

int BleControlRequestChannel::initAesCcm() {
    char key[AES_CCM_KEY_SIZE] = {};
    static_assert(sizeof(key) <= HAL_DEVICE_SECRET_SIZE, "");
    int ret = hal_get_device_secret(key, sizeof(key), nullptr);
    if (ret < 0) {
        return ret;
    }
    mbedtls_ccm_init(&aesCcm_);
    ret = mbedtls_ccm_setkey(&aesCcm_, MBEDTLS_CIPHER_ID_AES, (const uint8_t*)key, AES_CCM_KEY_SIZE * 8);
    if (ret != 0) {
        LOG(ERROR, "mbedtls_ccm_setkey() failed: %d", ret);
        return SYSTEM_ERROR_UNKNOWN;
    }
    return 0;
}

BleControlRequestChannel::Request* BleControlRequestChannel::allocRequest(size_t size) {
    const auto req = (Request*)malloc(sizeof(Request));
    if (!req) {
        return nullptr;
    }
    memset(req, 0, sizeof(Request));
    req->reqBuf = (char*)malloc(size + MESSAGE_HEADER_SIZE + REQUEST_HEADER_SIZE + AES_CCM_TAG_SIZE);
    if (!req->reqBuf) {
        free(req);
        return nullptr;
    }
    req->request_data = (size > 0) ? req->reqBuf + MESSAGE_HEADER_SIZE + REQUEST_HEADER_SIZE : nullptr;
    req->request_size = size;
    req->channel = this;
    return req;
}

BleControlRequestChannel::Request* BleControlRequestChannel::freeRequest(Request* req) {
    if (!req) {
        return nullptr;
    }
    const auto next = req->next;
    free(req->reqBuf);
    free(req->repBuf);
    free(req);
    return next;
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

} // particle::system

} // particle

#endif // SYSTEM_CONTROL_ENABLED && HAL_PLATFORM_BLE
