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

#include "timer_hal.h"
#include "deviceid_hal.h"

#include "endian.h"
#include "debug.h"

#include "mbedtls/ecjpake.h"
#include "mbedtls/ccm.h"

#include "mbedtls_util.h"

#ifndef PAIRING_ENABLED
#define PAIRING_ENABLED 0
#endif

#undef DEBUG // Legacy logging macro

#if DEBUG_CHANNEL
#define DEBUG(_fmt, ...) \
        do { \
            LOG_PRINTF(TRACE, _fmt "\r\n", ##__VA_ARGS__); \
        } while (false)

#define DEBUG_DUMP(_data, _size) \
        do { \
            LOG_PRINT(TRACE, "> "); \
            LOG_DUMP(TRACE, _data, _size); \
            LOG_PRINTF(TRACE, " (%u bytes)\r\n", (unsigned)(_size)); \
        } while (false)
#else
#define DEBUG(...)
#define DEBUG_DUMP(...)
#endif

namespace particle {

namespace system {

namespace {

// Header containing some of the fields common for request and reply messages. These fields are
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

// Handshake packet header
struct __attribute__((packed)) HandshakeHeader {
    uint16_t size; // Payload size
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

// Size of the buffer pool
const size_t BUFFER_POOL_SIZE = 1024;

// Size of the message header
const size_t MESSAGE_HEADER_SIZE = sizeof(MessageHeader);

// Size of the request header
const size_t REQUEST_HEADER_SIZE = sizeof(RequestHeader);

// Size of the reply header
const size_t REPLY_HEADER_SIZE = sizeof(ReplyHeader);

// Size of the handshake packet header
const size_t HANDSHAKE_HEADER_SIZE = sizeof(HandshakeHeader);

// Maximum size of the payload data in a handshake packet
const size_t MAX_HANDSHAKE_PAYLOAD_SIZE = 512;

// Handshake timeout in milliseconds
const unsigned HANDSHAKE_TIMEOUT = 15000;

// Size of the J-PAKE passphrase in bytes
const size_t JPAKE_PASSPHRASE_SIZE = 10;

// Size of the J-PAKE's shared secret in bytes
const size_t JPAKE_SHARED_SECRET_SIZE = 32;

// Size of the cipher's key in bytes
const size_t AES_CCM_KEY_SIZE = 16;

// Size of the authentication field in bytes
const size_t AES_CCM_TAG_SIZE = 8; // M

// Size of the length field in bytes
const size_t AES_CCM_LENGTH_SIZE = 4; // L

// Size of the nonce in bytes
const size_t AES_CCM_NONCE_SIZE = 11; // 15 - L

} // particle::system::

class BleControlRequestChannel::HandshakeHandler {
public:
    enum Result {
        DONE = 0,
        RUNNING
    };

    virtual ~HandshakeHandler() = default;

    virtual int run() = 0;

protected:
    explicit HandshakeHandler(BleControlRequestChannel* channel) :
            channel_(channel),
            size_(0),
            offs_(0) {
    }

    int init() {
        data_.reset(new(std::nothrow) char[MAX_HANDSHAKE_PAYLOAD_SIZE]);
        if (!data_) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        return 0;
    }

    void destroy() {
        data_.reset();
        size_ = 0;
        offs_ = 0;
    }

    int readPacket() {
        if (offs_ == size_) {
            // Read packet header
            HandshakeHeader h = {};
            if (!channel_->readAll((char*)&h, HANDSHAKE_HEADER_SIZE)) {
                return Result::RUNNING;
            }
            size_ = littleEndianToNative(h.size);
            if (size_ == 0 || size_ > MAX_HANDSHAKE_PAYLOAD_SIZE) {
                LOG_DEBUG(WARN, "Too large handshake packet");
                return SYSTEM_ERROR_TOO_LARGE;
            }
            offs_ = 0;
        }
        // Read remaining packet data
        offs_ += channel_->readSome(data_.get() + offs_, size_ - offs_);
        if (offs_ < size_) {
            return Result::RUNNING;
        }
        return Result::DONE;
    }

    void writePacket(Buffer* buf) {
        // Serialize packet header
        HandshakeHeader h = {};
        h.size = nativeToLittleEndian(buf->size);
        buf->data -= HANDSHAKE_HEADER_SIZE;
        buf->size += HANDSHAKE_HEADER_SIZE;
        memcpy(buf->data, &h, HANDSHAKE_HEADER_SIZE);
        // Enqueue the buffer for sending
        channel_->sendBuffer(buf);
    }

    int allocPacket(Buffer** buf) {
        Buffer* b = nullptr;
        const int ret = channel_->reallocBuffer(HANDSHAKE_HEADER_SIZE + MAX_HANDSHAKE_PAYLOAD_SIZE, &b);
        if (ret != 0) {
            return ret;
        }
        b->data += HANDSHAKE_HEADER_SIZE;
        b->size -= HANDSHAKE_HEADER_SIZE;
        *buf = b;
        return 0;
    }

    void freePacket(Buffer* buf) {
        channel_->freeBuffer(buf);
    }

    const char* packetData() const {
        return data_.get();
    }

    size_t packetSize() const {
        return size_;
    }

private:
    BleControlRequestChannel* channel_;
    std::unique_ptr<char[]> data_; // Received payload data
    size_t size_;
    size_t offs_;
};

class BleControlRequestChannel::JpakeHandler: public HandshakeHandler {
public:
    explicit JpakeHandler(BleControlRequestChannel* channel) :
            HandshakeHandler(channel),
            ctx_(),
            state_(State::NEW) {
    }

    ~JpakeHandler() {
        mbedtls_ecjpake_free(&ctx_);
    }

    int init(const char* key, size_t keySize) {
        int ret = HandshakeHandler::init();
        if (ret != 0) {
            return ret;
        }
        mbedtls_ecjpake_init(&ctx_);
        ret = mbedtls_ecjpake_setup(&ctx_, MBEDTLS_ECJPAKE_SERVER, MBEDTLS_MD_SHA256, MBEDTLS_ECP_DP_SECP256R1,
                (const unsigned char*)key, keySize);
        if (ret != 0) {
            LOG(ERROR, "mbedtls_ecjpake_setup() failed: %d", ret);
            return SYSTEM_ERROR_UNKNOWN;
        }
        state_ = State::READ_ROUND1;
        return 0;
    }

    virtual int run() override {
        int ret = 0;
        switch (state_) {
        case State::READ_ROUND1:
            ret = readRound1();
            break;
        case State::WRITE_ROUND1:
            ret = writeRound1();
            break;
        case State::READ_ROUND2:
            ret = readRound2();
            break;
        case State::WRITE_ROUND2:
            ret = writeRound2();
            break;
        case State::DONE:
            return Result::DONE;
        default:
            return SYSTEM_ERROR_INVALID_STATE;
        }
        if (ret < 0) {
            state_ = State::FAILED;
        }
        return ret;
    }

    int genSecret(char* buf, size_t size) {
        if (state_ != State::DONE) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        size_t n = 0;
        const int ret = mbedtls_ecjpake_derive_secret(&ctx_, (unsigned char*)buf, size, &n, mbedtls_default_rng, nullptr);
        if (ret != 0) {
            LOG(ERROR, "mbedtls_ecjpake_derive_secret() failed: %d", ret);
            return SYSTEM_ERROR_UNKNOWN;
        }
        return n;
    }

private:
    // TODO: Add a verification step
    enum class State {
        NEW,
        READ_ROUND1,
        WRITE_ROUND1,
        READ_ROUND2,
        WRITE_ROUND2,
        DONE,
        FAILED
    };

    mbedtls_ecjpake_context ctx_;
    State state_;

    int readRound1() {
        int ret = readPacket();
        if (ret != 0) {
            return ret;
        }
        ret = mbedtls_ecjpake_read_round_one(&ctx_, (const unsigned char*)packetData(), packetSize());
        if (ret != 0) {
            LOG(ERROR, "mbedtls_ecjpake_read_round_one() failed: %d", ret);
            return SYSTEM_ERROR_BAD_DATA;
        }
        state_ = State::WRITE_ROUND1;
        return Result::RUNNING;
    }

    int writeRound1() {
        Buffer* buf = nullptr;
        int ret = allocPacket(&buf);
        if (ret != 0) {
            return ret;
        }
        size_t n = 0;
        ret = mbedtls_ecjpake_write_round_one(&ctx_, (unsigned char*)buf->data, buf->size, &n, mbedtls_default_rng, nullptr);
        if (ret != 0) {
            freePacket(buf);
            LOG(ERROR, "mbedtls_ecjpake_write_round_one() failed: %d", ret);
            return SYSTEM_ERROR_UNKNOWN;
        }
        buf->size = n;
        writePacket(buf);
        state_ = State::WRITE_ROUND2; // Send the 2nd round immediately
        return Result::RUNNING;
    }

    int readRound2() {
        int ret = readPacket();
        if (ret != 0) {
            return ret;
        }
        ret = mbedtls_ecjpake_read_round_two(&ctx_, (const unsigned char*)packetData(), packetSize());
        if (ret != 0) {
            LOG(ERROR, "mbedtls_ecjpake_read_round_two() failed: %d", ret);
            return SYSTEM_ERROR_BAD_DATA;
        }
        HandshakeHandler::destroy();
        state_ = State::DONE;
        return Result::DONE;
    }

    int writeRound2() {
        Buffer* buf = nullptr;
        int ret = allocPacket(&buf);
        if (ret != 0) {
            return ret;
        }
        size_t n = 0;
        ret = mbedtls_ecjpake_write_round_two(&ctx_, (unsigned char*)buf->data, buf->size, &n, mbedtls_default_rng, nullptr);
        if (ret != 0) {
            freePacket(buf);
            LOG(ERROR, "mbedtls_ecjpake_write_round_two() failed: %d", ret);
            return SYSTEM_ERROR_UNKNOWN;
        }
        buf->size = n;
        writePacket(buf);
        state_ = State::READ_ROUND2;
        return Result::RUNNING;
    }
};

class BleControlRequestChannel::AesCcmCipher {
public:
    AesCcmCipher() :
            clientCtx_(),
            serverCtx_(),
            reqCount_(0),
            repCount_(0) {
    }

    ~AesCcmCipher() {
        mbedtls_ccm_free(&clientCtx_);
        mbedtls_ccm_free(&serverCtx_);
    }

    int init(const char* clientKey, const char* serverKey) {
        int ret = initContext(&clientCtx_, clientKey);
        if (ret != 0) {
            return ret;
        }
        ret = initContext(&serverCtx_, serverKey);
        if (ret != 0) {
            return ret;
        }
        return 0;
    }

    int decryptRequestData(char* buf, size_t payloadSize) {
        uint8_t nonce[AES_CCM_NONCE_SIZE] = {};
        genNonce(nonce, ++reqCount_, MessageType::REQUEST);
        const int ret = mbedtls_ccm_auth_decrypt(&clientCtx_,
                payloadSize + REQUEST_HEADER_SIZE, // Size of the input data
                nonce, AES_CCM_NONCE_SIZE, // Nonce
                (const uint8_t*)buf, MESSAGE_HEADER_SIZE, // Additional data
                (const uint8_t*)buf + MESSAGE_HEADER_SIZE, // Input buffer
                (uint8_t*)buf + MESSAGE_HEADER_SIZE, // Output buffer
                (const uint8_t*)buf + payloadSize + MESSAGE_HEADER_SIZE + REQUEST_HEADER_SIZE, AES_CCM_TAG_SIZE); // Authentication tag
        if (ret != 0) {
            LOG(ERROR, "mbedtls_ccm_auth_decrypt() failed: %d", ret);
            return SYSTEM_ERROR_BAD_DATA;
        }
        return 0;
    }

    int encryptReplyData(char* buf, size_t payloadSize) {
        uint8_t nonce[AES_CCM_NONCE_SIZE] = {};
        genNonce(nonce, ++repCount_, MessageType::REPLY);
        const int ret = mbedtls_ccm_encrypt_and_tag(&serverCtx_,
                payloadSize + REPLY_HEADER_SIZE, // Size of the input data
                nonce, AES_CCM_NONCE_SIZE, // Nonce
                (const uint8_t*)buf, MESSAGE_HEADER_SIZE, // Additional data
                (const uint8_t*)buf + MESSAGE_HEADER_SIZE, // Input buffer
                (uint8_t*)buf + MESSAGE_HEADER_SIZE, // Output buffer
                (uint8_t*)buf + payloadSize + MESSAGE_HEADER_SIZE + REPLY_HEADER_SIZE, AES_CCM_TAG_SIZE); // Authentication tag
        if (ret != 0) {
            LOG(ERROR, "mbedtls_ccm_encrypt_and_tag() failed: %d", ret);
            return SYSTEM_ERROR_UNKNOWN;
        }
        return 0;
    }

private:
    enum MessageType {
        REQUEST = 0x01,
        REPLY = 0x02
    };

    mbedtls_ccm_context clientCtx_;
    mbedtls_ccm_context serverCtx_;
    uint32_t reqCount_;
    uint32_t repCount_;

    static int initContext(mbedtls_ccm_context* ctx, const char* key) {
        mbedtls_ccm_init(ctx);
        const int ret = mbedtls_ccm_setkey(ctx, MBEDTLS_CIPHER_ID_AES, (const uint8_t*)key, AES_CCM_KEY_SIZE * 8);
        if (ret != 0) {
            LOG(ERROR, "mbedtls_ccm_setkey() failed: %d", ret);
            return SYSTEM_ERROR_UNKNOWN;
        }
        return 0;
    }

    static void genNonce(uint8_t* nonce, uint32_t counter, MessageType msgType) {
        // TODO: RFC 3610 suggests using an endpoint's address as part of the nonce. TLS generates a
        // random implicit part of the nonce during the handshake
        memset(nonce, 0xff, 6);
        nonce[6] = (counter >> 0) & 0xff;
        nonce[7] = (counter >> 8) & 0xff;
        nonce[8] = (counter >> 16) & 0xff;
        nonce[9] = (counter >> 24) & 0xff;
        nonce[10] = (uint8_t)msgType;
    }
};

BleControlRequestChannel::BleControlRequestChannel(ControlRequestHandler* handler) :
        ControlRequestChannel(handler),
#if DEBUG_CHANNEL
        allocReqCount_(0),
        heapBufCount_(0),
        poolBufCount_(0),
#endif
        inBufSize_(0),
        curReq_(nullptr),
        reqBufSize_(0),
        reqBufOffs_(0),
        packetSize_(0),
        connStartTime_(0),
        connHandle_(BLE_INVALID_CONN_HANDLE),
        curConnHandle_(BLE_INVALID_CONN_HANDLE),
        connId_(0),
        curConnId_(0),
        maxPacketSize_(0),
        notifEnabled_(false),
        writable_(false),
        sendCharHandle_(BLE_INVALID_ATTR_HANDLE),
        recvCharHandle_(BLE_INVALID_ATTR_HANDLE) {
}

BleControlRequestChannel::~BleControlRequestChannel() {
    destroy();
}

int BleControlRequestChannel::init() {
    // Make sure we have a copy of the device secret in the DCT, so that it can be easily extracted
    // via dfu-util for testing purposes
    hal_get_device_secret(nullptr, 0, nullptr);
    // Initialize the BLE profile
    int ret = initProfile();
    if (ret != 0) {
        goto error;
    }
    // TODO: Initialize this allocator when a BLE connection is accepted
    ret = pool_.init(BUFFER_POOL_SIZE);
    if (ret != 0) {
        goto error;
    }
    return 0;
error:
    destroy();
    return ret;
}

void BleControlRequestChannel::destroy() {
    // TODO: There doesn't seem to be a straightforward way to uninitialize the profile
}

void BleControlRequestChannel::run() {
    int ret = 0;
    const auto connId = curConnId_.load(std::memory_order_acquire);
    if (connId_ != connId) {
        const auto prevConnHandle = connHandle_;
        connHandle_ = curConnHandle_;
        connId_ = connId;
        // Reset channel state
        resetChannel();
        if (connHandle_ != BLE_INVALID_CONN_HANDLE) {
            LOG(TRACE, "Connected");
            ret = initChannel();
            if (ret != 0) {
                goto error;
            }
        } else if (prevConnHandle != BLE_INVALID_CONN_HANDLE) {
            LOG(TRACE, "Disconnected");
        }
    }
    if (connHandle_ != BLE_INVALID_CONN_HANDLE) {
        if (jpake_) {
            // Process handshake packets
            ret = jpake_->run();
            if (ret < 0) {
                LOG(ERROR, "Handshake failed");
                goto error;
            }
            if (ret == JpakeHandler::DONE) {
                ret = initAesCcm();
                if (ret != 0) {
                    goto error;
                }
                jpake_.reset();
                LOG(TRACE, "Handshake done");
            }
        } else {
            // Serialize next reply and enqueue it for sending
            ret = sendReply();
            if (ret != 0) {
                goto error;
            }
            // Receive next request
            ret = receiveRequest();
            if (ret != 0) {
                goto error;
            }
        }
        // TODO: The handshake handler doesn't perform verification of the shared secret. For now,
        // we require the client to send its first request within the handshake timeout, and use
        // the authentication tag of that request to verify the key exchange
        if (connStartTime_ != 0 && HAL_Timer_Get_Milli_Seconds() - connStartTime_ >= HANDSHAKE_TIMEOUT) {
            LOG(ERROR, "Handshake timeout");
            ret = SYSTEM_ERROR_TIMEOUT;
            goto error;
        }
        // Send BLE notification packet
        ret = sendPacket();
        if (ret != 0) {
            goto error;
        }
    }
    return;
error:
    LOG(ERROR, "Channel error: %d", ret);
    if (connHandle_ != BLE_INVALID_CONN_HANDLE) {
        ble_disconnect(connHandle_, nullptr);
        connHandle_ = BLE_INVALID_CONN_HANDLE;
    }
    resetChannel();
}

int BleControlRequestChannel::allocReplyData(ctrl_request* ctrlReq, size_t size) {
    const auto req = static_cast<Request*>(ctrlReq);
    const int ret = reallocBuffer(size + MESSAGE_HEADER_SIZE + REPLY_HEADER_SIZE + AES_CCM_TAG_SIZE, &req->repBuf);
    if (ret != 0) {
        return ret;
    }
    if (size > 0) {
        req->reply_data = req->repBuf->data + MESSAGE_HEADER_SIZE + REPLY_HEADER_SIZE;
    } else {
        req->reply_data = nullptr;
    }
    req->reply_size = size;
    return 0;
}

void BleControlRequestChannel::freeRequestData(ctrl_request* ctrlReq) {
    const auto req = static_cast<Request*>(ctrlReq);
    delete[] req->reqBuf;
    req->reqBuf = nullptr;
    req->request_data = nullptr;
    req->request_size = 0;
}

void BleControlRequestChannel::setResult(ctrl_request* ctrlReq, int result, ctrl_completion_handler_fn handler, void* data) {
    // TODO: Completion handling
    const auto req = static_cast<Request*>(ctrlReq);
    freeRequestData(req);
    req->result = result;
    const std::lock_guard<Mutex> lock(readyReqsLock_);
    readyReqs_.pushBack(req);
}

int BleControlRequestChannel::initChannel() {
    int ret = 0;
    packetBuf_.reset(new(std::nothrow) char[BLE_MAX_ATTR_VALUE_PACKET_SIZE]);
    if (!packetBuf_) {
        ret = SYSTEM_ERROR_NO_MEMORY;
        goto error;
    }
    ret = initJpake();
    if (ret != 0) {
        goto error;
    }
    connStartTime_ = HAL_Timer_Get_Milli_Seconds();
    return 0;
error:
    resetChannel();
    return ret;
}

void BleControlRequestChannel::resetChannel() {
    jpake_.reset();
    aesCcm_.reset();
    packetBuf_.reset();
    while (Request* req = readyReqs_.popFront()) {
        freeRequest(req);
    }
    while (Buffer* buf = outBufs_.popFront()) {
        freeBuffer(buf);
    }
    while (Buffer* buf = readInBufs_.popFront()) {
        freePooledBuffer(buf);
    }
    freeRequest(curReq_);
    curReq_ = nullptr;
    reqBufSize_ = 0;
    reqBufOffs_ = 0;
    inBufSize_ = 0;
    packetSize_ = 0;
    connStartTime_ = 0;
}

int BleControlRequestChannel::receiveRequest() {
    if (!curReq_) {
        // Read message header
        MessageHeader mh = {};
        if (!readAll((char*)&mh, MESSAGE_HEADER_SIZE)) {
            return 0; // Wait for more data
        }
        // Allocate a request object
        const size_t payloadSize = littleEndianToNative(mh.size);
        const int ret = allocRequest(payloadSize, &curReq_);
        if (ret != 0) {
            LOG(ERROR, "Unable to allocate request object");
            return ret;
        }
        memcpy(curReq_->reqBuf, &mh, MESSAGE_HEADER_SIZE);
        reqBufSize_ = payloadSize + MESSAGE_HEADER_SIZE + REQUEST_HEADER_SIZE + AES_CCM_TAG_SIZE; // Total size of the request data
        reqBufOffs_ = MESSAGE_HEADER_SIZE;
    }
    // Read remaining request data
    const auto p = curReq_->reqBuf;
    const size_t n = readSome(p + reqBufOffs_, reqBufSize_ - reqBufOffs_);
    reqBufOffs_ += n;
    if (reqBufOffs_ < reqBufSize_) {
        return 0; // Wait for more data
    }
    // Decrypt request data
    SPARK_ASSERT(aesCcm_);
    const int ret = aesCcm_->decryptRequestData(p, curReq_->request_size);
    if (ret != 0) {
        return ret;
    }
    // Parse request header
    RequestHeader rh = {};
    memcpy(&rh, p + MESSAGE_HEADER_SIZE, REQUEST_HEADER_SIZE);
    curReq_->id = littleEndianToNative(rh.id); // Request ID
    curReq_->type = littleEndianToNative(rh.type); // Request type
    LOG(TRACE, "Received a request message, ID: %u", (unsigned)curReq_->id);
    // Process request
    handler()->processRequest(curReq_, this);
    curReq_ = nullptr;
    reqBufSize_ = 0;
    reqBufOffs_ = 0;
    // Reset handshake timer (see notes in the run() method)
    connStartTime_ = 0;
    return 0;
}

int BleControlRequestChannel::sendReply() {
    std::unique_lock<Mutex> lock(readyReqsLock_);
    Request* req = nullptr;
    while ((req = readyReqs_.popFront())) {
        if (req->connId == connId_) {
            break;
        }
        freeRequest(req);
    }
    lock.unlock();
    if (!req) {
        return 0; // Nothing to send
    }
    // Make sure we have a buffer to serialize the reply
    if (!req->repBuf) {
        const int ret = reallocBuffer(MESSAGE_HEADER_SIZE + REPLY_HEADER_SIZE + AES_CCM_TAG_SIZE, &req->repBuf);
        if (ret != 0) {
            LOG(ERROR, "Unable to allocate buffer for reply data");
            freeRequest(req);
            return ret;
        }
    }
    const auto p = req->repBuf->data;
    // Serialize message header
    MessageHeader mh = {};
    mh.size = nativeToLittleEndian(req->reply_size);
    memcpy(p, &mh, MESSAGE_HEADER_SIZE);
    // Serialize reply header
    ReplyHeader rh = {};
    rh.id = nativeToLittleEndian(req->id);
    rh.result = nativeToLittleEndian(req->result);
    memcpy(p + MESSAGE_HEADER_SIZE, &rh, REPLY_HEADER_SIZE);
    // Encrypt reply data
    SPARK_ASSERT(aesCcm_);
    const int ret = aesCcm_->encryptReplyData(p, req->reply_size);
    if (ret == 0) {
        // Enqueue the reply buffer for sending
        outBufs_.pushBack(req->repBuf);
        req->repBuf = nullptr;
        LOG(TRACE, "Enqueued a reply message for sending, ID: %u", (unsigned)req->id);
    }
    freeRequest(req);
    return ret;
}

int BleControlRequestChannel::sendPacket() {
    if (!writable_) {
        return 0; // Can't send now
    }
    // Prepare a BLE packet
    SPARK_ASSERT(packetBuf_);
    const size_t maxSize = maxPacketSize_;
    Buffer* buf = nullptr;
    while (packetSize_ < maxSize && (buf = outBufs_.front())) {
        const size_t n = std::min(maxSize - packetSize_, buf->size);
        memcpy(packetBuf_.get() + packetSize_, buf->data, n);
        buf->data += n;
        buf->size -= n;
        if (buf->size == 0) {
            outBufs_.popFront();
            freeBuffer(buf);
        }
        packetSize_ += n;
    }
    if (packetSize_ == 0) {
        return 0; // Nothing to send
    }
    // Send packet
    const int ret = ble_set_char_value(connHandle_, sendCharHandle_, packetBuf_.get(), packetSize_,
            BLE_SET_CHAR_VALUE_FLAG_NOTIFY, nullptr);
    if (ret == BLE_ERROR_BUSY) {
        writable_ = false; // Retry later
        return 0;
    }
    if (ret != (int)packetSize_) {
        LOG(ERROR, "ble_set_char_value() failed: %d", ret);
        return ret;
    }
    DEBUG("Sent BLE packet");
    DEBUG_DUMP(packetBuf_.get(), packetSize_);
    packetSize_ = 0;
    return 0;
}

bool BleControlRequestChannel::readAll(char* data, size_t size) {
    // Keep taking input buffers from the queue until there's enough data
    Buffer* buf = nullptr;
    while (inBufSize_ < size && (buf = inBufs_.popFront())) {
        readInBufs_.pushBack(buf);
        inBufSize_ += buf->size;
    }
    if (inBufSize_ < size) {
        return false; // Wait for more data
    }
    // Copy data to the destination buffer
    DEBUG("Reading %u bytes", (unsigned)size);
    size_t offs = 0;
    while (offs < size) {
        buf = readInBufs_.front();
        SPARK_ASSERT(buf);
        const size_t n = std::min(size - offs, buf->size);
        memcpy(data + offs, buf->data, n);
        buf->size -= n;
        if (buf->size == 0) {
            // Free the drained buffer
            readInBufs_.popFront();
            freePooledBuffer(buf);
        } else {
            buf->data += n;
        }
        offs += n;
    }
    inBufSize_ -= size;
    DEBUG_DUMP(data, size);
    return true;
}

size_t BleControlRequestChannel::readSome(char* data, size_t size) {
    Buffer* buf = nullptr;
    while (inBufSize_ < size && (buf = inBufs_.popFront())) {
        readInBufs_.pushBack(buf);
        inBufSize_ += buf->size;
    }
    if (inBufSize_ < size) {
        size = inBufSize_;
    }
    if (size > 0) {
        const bool ok = readAll(data, size);
        SPARK_ASSERT(ok);
    }
    return size;
}

int BleControlRequestChannel::connected(const ble_connected_event_data& event) {
    curConnHandle_ = event.conn_handle;
    // Get initial connection parameters
    ble_conn_param connParam = { .version = BLE_API_VERSION };
    int ret = ble_get_conn_param(curConnHandle_, &connParam, nullptr);
    if (ret == 0) {
        maxPacketSize_ = connParam.att_mtu_size - BLE_ATT_OPCODE_SIZE - BLE_ATT_HANDLE_SIZE;
    } else {
        maxPacketSize_ = BLE_MIN_ATTR_VALUE_PACKET_SIZE;
    }
    ble_char_param charParam = { .version = BLE_API_VERSION };
    ret = ble_get_char_param(curConnHandle_, sendCharHandle_, &charParam, nullptr);
    if (ret == 0) {
        notifEnabled_ = charParam.notif_enabled;
    } else {
        notifEnabled_ = false;
    }
    writable_ = notifEnabled_;
    // Update connection state counter
    curConnId_.fetch_add(1, std::memory_order_release);
    return 0;
}

int BleControlRequestChannel::disconnected(const ble_disconnected_event_data& event) {
    // Free queued buffers
    while (Buffer* buf = inBufs_.popFront()) {
        freePooledBuffer(buf);
    }
    // Reset connection parameters
    writable_ = false;
    notifEnabled_ = false;
    maxPacketSize_ = 0;
    curConnHandle_ = BLE_INVALID_CONN_HANDLE;
    // Update connection state counter
    curConnId_.fetch_add(1, std::memory_order_release);
    return 0;
}

int BleControlRequestChannel::connParamChanged(const ble_conn_param_changed_event_data& event) {
    ble_conn_param param = { .version = BLE_API_VERSION };
    const int ret = ble_get_conn_param(curConnHandle_, &param, nullptr);
    if (ret != 0) {
        return ret;
    }
    maxPacketSize_ = param.att_mtu_size - BLE_ATT_OPCODE_SIZE - BLE_ATT_HANDLE_SIZE;
    return 0;
}

int BleControlRequestChannel::charParamChanged(const ble_char_param_changed_event_data& event) {
    if (event.char_handle == sendCharHandle_) {
        ble_char_param param = { .version = BLE_API_VERSION };
        const int ret = ble_get_char_param(curConnHandle_, sendCharHandle_, &param, nullptr);
        if (ret != 0) {
            return ret;
        }
        notifEnabled_ = param.notif_enabled;
        writable_ = notifEnabled_;
    }
    return 0;
}

int BleControlRequestChannel::dataSent(const ble_data_sent_event_data& event) {
    if (notifEnabled_) {
        writable_ = true;
    }
    return 0;
}

int BleControlRequestChannel::dataReceived(const ble_data_received_event_data& event) {
    if (event.char_handle == recvCharHandle_) {
        DEBUG("Received BLE packet");
        DEBUG_DUMP(event.data, event.size);
        Buffer* buf = nullptr;
        const int ret = allocPooledBuffer(event.size, &buf);
        if (ret != 0) {
            return ret;
        }
        memcpy(buf->data, event.data, event.size);
        inBufs_.pushBack(buf);
    }
    return 0;
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
    SPARK_ASSERT(jpake_);
    char secret[JPAKE_SHARED_SECRET_SIZE] = {};
    static_assert(sizeof(secret) >= AES_CCM_KEY_SIZE * 2, "");
    int ret = jpake_->genSecret(secret, sizeof(secret));
    if (ret < 0) {
        return ret;
    }
    if (ret < (int)sizeof(secret)) {
        LOG(ERROR, "Invalid size of the shared secret data");
        return SYSTEM_ERROR_INTERNAL;
    }
    std::unique_ptr<AesCcmCipher> c(new(std::nothrow) AesCcmCipher);
    if (!c) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    // Split the shared secret into client and server keys
    ret = c->init(secret, secret + AES_CCM_KEY_SIZE);
    if (ret != 0) {
        return ret;
    }
    aesCcm_.reset(c.release()); // Transfer ownership
    return 0;
}

int BleControlRequestChannel::initJpake() {
    char secret[JPAKE_PASSPHRASE_SIZE] = {};
    static_assert(sizeof(secret) <= HAL_DEVICE_SECRET_SIZE, "");
    int ret = hal_get_device_secret(secret, sizeof(secret), nullptr);
    if (ret < 0) {
        return ret;
    }
    if (ret < (int)sizeof(secret)) {
        LOG(ERROR, "Invalid size of the device secret data");
        return SYSTEM_ERROR_INTERNAL;
    }
    std::unique_ptr<JpakeHandler> h(new(std::nothrow) JpakeHandler(this));
    if (!h) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    ret = h->init(secret, sizeof(secret));
    if (ret != 0) {
        return ret;
    }
    jpake_.reset(h.release()); // Transfer ownership
    return 0;
}

int BleControlRequestChannel::allocRequest(size_t size, Request** req) {
    std::unique_ptr<Request> r(new(std::nothrow) Request());
    if (!r) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    r->reqBuf = new(std::nothrow) char[size + MESSAGE_HEADER_SIZE + REQUEST_HEADER_SIZE + AES_CCM_TAG_SIZE];
    if (!r->reqBuf) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    if (size > 0) {
        r->request_data = r->reqBuf + MESSAGE_HEADER_SIZE + REQUEST_HEADER_SIZE;
    }
    r->request_size = size;
    r->connId = connId_;
    r->channel = this;
    *req = r.release(); // Transfer ownership
#if DEBUG_CHANNEL
    const auto count = ++allocReqCount_;
    DEBUG("Allocated a request object (count: %u)", (unsigned)count);
#endif
    return 0;
}

void BleControlRequestChannel::freeRequest(Request* req) {
    if (req) {
        freeBuffer(req->repBuf);
        delete[] req->reqBuf;
        delete req;
#if DEBUG_CHANNEL
        const auto count = --allocReqCount_;
        DEBUG("Freed a request object (count: %u)", (unsigned)count);
#endif
    }
}

int BleControlRequestChannel::reallocBuffer(size_t size, Buffer** buf) {
    const auto b = reallocLinkedBuffer<Buffer>(*buf, size);
    if (!b) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    b->data = linkedBufferData(b);
    b->size = size;
#if DEBUG_CHANNEL
    if (!*buf) {
        const auto count = ++heapBufCount_;
        DEBUG("Allocated a buffer, size: %u (count: %u)", (unsigned)size, (unsigned)count);
    }
#endif
    *buf = b;
    return 0;
}

void BleControlRequestChannel::freeBuffer(Buffer* buf) {
    if (buf) {
        freeLinkedBuffer(buf);
#if DEBUG_CHANNEL
        const auto count = --heapBufCount_;
        DEBUG("Freed a buffer (count: %u)", (unsigned)count);
#endif
    }
}

int BleControlRequestChannel::allocPooledBuffer(size_t size, Buffer** buf) {
    const auto b = allocLinkedBuffer<Buffer>(size, &pool_);
    if (!b) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    b->data = linkedBufferData(b);
    b->size = size;
    *buf = b;
#if DEBUG_CHANNEL
    const auto count = ++poolBufCount_;
    DEBUG("Allocated a pooled buffer, size: %u (count: %u)", (unsigned)size, (unsigned)count);
#endif
    return 0;
}

void BleControlRequestChannel::freePooledBuffer(Buffer* buf) {
    if (buf) {
        freeLinkedBuffer(buf, &pool_);
#if DEBUG_CHANNEL
        const auto count = --poolBufCount_;
        DEBUG("Freed a pooled buffer (count: %u)", (unsigned)count);
#endif
    }
}

// Note: This method is called from an ISR
void BleControlRequestChannel::processBleEvent(int event, const void* eventData, void* userData) {
    const auto ch = (BleControlRequestChannel*)userData;
    int ret = 0;
    switch (event) {
    case BLE_EVENT_CONNECTED: {
        const auto d = (const ble_connected_event_data*)eventData;
        ret = ch->connected(*d);
        break;
    }
    case BLE_EVENT_DISCONNECTED: {
        const auto d = (const ble_disconnected_event_data*)eventData;
        ret = ch->disconnected(*d);
        break;
    }
    case BLE_EVENT_CONN_PARAM_CHANGED: {
        const auto d = (const ble_conn_param_changed_event_data*)eventData;
        ret = ch->connParamChanged(*d);
        break;
    }
    case BLE_EVENT_CHAR_PARAM_CHANGED: {
        const auto d = (const ble_char_param_changed_event_data*)eventData;
        ret = ch->charParamChanged(*d);
        break;
    }
    case BLE_EVENT_DATA_SENT: {
        const auto d = (const ble_data_sent_event_data*)eventData;
        ret = ch->dataSent(*d);
        break;
    }
    case BLE_EVENT_DATA_RECEIVED: {
        const auto d = (const ble_data_received_event_data*)eventData;
        ret = ch->dataReceived(*d);
        break;
    }
    default:
        break;
    }
    if (ret != 0) {
        LOG(ERROR, "Failed to process BLE event: %d (error: %d)", event, ret);
        if (ch->curConnHandle_ != BLE_INVALID_CONN_HANDLE) {
            ble_disconnect(ch->curConnHandle_, nullptr);
        }
    }
}

} // particle::system

} // particle

#endif // SYSTEM_CONTROL_ENABLED && HAL_PLATFORM_BLE
