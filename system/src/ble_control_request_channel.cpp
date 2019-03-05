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

#include "scope_guard.h"
#include "endian_util.h"
#include "debug.h"

#include "mbedtls/ecjpake.h"
#include "mbedtls/ccm.h"
#include "mbedtls/md.h"

#include "mbedtls_util.h"

#define CHECK(_expr) \
        do { \
            const int _ret = _expr; \
            if (_ret < 0) { \
                return _ret; \
            } \
        } while (false)

#define CHECK_MBEDTLS(_expr) \
        do { \
            const int _ret = _expr; \
            if (_ret != 0) { \
                LOG_DEBUG(ERROR, #_expr " failed: %d", _ret); \
                return mbedtlsError(_ret); \
            } \
        } while (false)

#undef DEBUG // Legacy logging macro

#if BLE_CHANNEL_DEBUG_ENABLED
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
const unsigned HANDSHAKE_TIMEOUT = 10000;

// Size of the J-PAKE passphrase in bytes
const size_t JPAKE_PASSPHRASE_SIZE = 15;

// Size of the J-PAKE's shared secret in bytes
const size_t JPAKE_SHARED_SECRET_SIZE = 32;

// Client identity string
const char* const JPAKE_CLIENT_ID = "client";

// Server identity string
const char* const JPAKE_SERVER_ID = "server";

// Size of the cipher's key in bytes
const size_t AES_CCM_KEY_SIZE = 16;

// Size of the authentication field in bytes
const size_t AES_CCM_TAG_SIZE = 8;

// Total size of the nonce in bytes
const size_t AES_CCM_NONCE_SIZE = 12;

// Size of the fixed part of the nonce in bytes
const size_t AES_CCM_FIXED_NONCE_SIZE = 8;

// Sanity checks
static_assert(15 - AES_CCM_NONCE_SIZE >= 3, // At least 3 bytes should be available to store the size of encrypted data
        "Invalid size of the CCM length field"); // See RFC 3610

static_assert(AES_CCM_NONCE_SIZE >= AES_CCM_FIXED_NONCE_SIZE + 4, // 4 bytes of the nonce data are reserved for the counter
        "Invalid size of the nonce");

static_assert(JPAKE_SHARED_SECRET_SIZE >= AES_CCM_KEY_SIZE + AES_CCM_FIXED_NONCE_SIZE * 2, // See BleControlRequestChannel::initAesCcm()
        "Invalid size of the shared secret");

#if BLE_CHANNEL_SECURITY_ENABLED
const size_t MESSAGE_FOOTER_SIZE = AES_CCM_TAG_SIZE;
#else
const size_t MESSAGE_FOOTER_SIZE = 0;
#endif

int mbedtlsError(int ret) {
    switch (ret) {
    case 0:
        return SYSTEM_ERROR_NONE;
    // TODO
    default:
        return SYSTEM_ERROR_UNKNOWN;
    }
}

class Sha256 {
public:
    static const size_t SIZE = 32;

    Sha256() :
            ctx_() {
    }

    ~Sha256() {
        destroy();
    }

    int init() {
        const auto mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
        if (!mdInfo) {
            LOG_DEBUG(ERROR, "mbedtls_md_info_from_type() failed");
            return SYSTEM_ERROR_NOT_FOUND;
        }
        mbedtls_md_init(&ctx_);
        CHECK_MBEDTLS(mbedtls_md_setup(&ctx_, mdInfo, 0 /* hmac */));
        CHECK_MBEDTLS(mbedtls_md_starts(&ctx_));
        return 0;
    }

    int init(const Sha256& src) {
        CHECK(init());
        CHECK_MBEDTLS(mbedtls_md_clone(&ctx_, &src.ctx_));
        return 0;
    }

    void destroy() {
        mbedtls_md_free(&ctx_);
    }

    int start() {
        CHECK_MBEDTLS(mbedtls_md_starts(&ctx_));
        return 0;
    }

    int update(const char* data, size_t size) {
        CHECK_MBEDTLS(mbedtls_md_update(&ctx_, (const uint8_t*)data, size));
        return 0;
    }

    int update(const char* str) {
        return update(str, strlen(str));
    }

    int finish(char* buf) {
        CHECK_MBEDTLS(mbedtls_md_finish(&ctx_, (uint8_t*)buf));
        return 0;
    }

private:
    mbedtls_md_context_t ctx_;
};

class HmacSha256 {
public:
    static const size_t SIZE = 32;

    HmacSha256() :
            ctx_() {
    }

    ~HmacSha256() {
        destroy();
    }

    int init(const char* key, size_t keySize) {
        const auto mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
        if (!mdInfo) {
            LOG_DEBUG(ERROR, "mbedtls_md_info_from_type() failed");
            return SYSTEM_ERROR_NOT_FOUND;
        }
        mbedtls_md_init(&ctx_);
        CHECK_MBEDTLS(mbedtls_md_setup(&ctx_, mdInfo, 1 /* hmac */));
        CHECK_MBEDTLS(mbedtls_md_hmac_starts(&ctx_, (const uint8_t*)key, keySize));
        return 0;
    }

    void destroy() {
        mbedtls_md_free(&ctx_);
    }

    int start() {
        CHECK_MBEDTLS(mbedtls_md_hmac_reset(&ctx_));
        return 0;
    }

    int update(const char* data, size_t size) {
        CHECK_MBEDTLS(mbedtls_md_hmac_update(&ctx_, (const uint8_t*)data, size));
        return 0;
    }

    int update(const char* str) {
        return update(str, strlen(str));
    }

    int finish(char* buf) {
        CHECK_MBEDTLS(mbedtls_md_hmac_finish(&ctx_, (uint8_t*)buf));
        return 0;
    }

private:
    mbedtls_md_context_t ctx_;
};

} // particle::system::

class BleControlRequestChannel::HandshakeHandler {
public:
    enum Result {
        DONE = 0,
        RUNNING
    };

    virtual ~HandshakeHandler() {
        destroy();
    }

    virtual int run() = 0;

protected:
    explicit HandshakeHandler(BleControlRequestChannel* channel) :
            channel_(channel),
            buf_(nullptr),
            timeStart_(0),
            size_(0),
            offs_(0) {
    }

    int init() {
        data_.reset(new(std::nothrow) char[MAX_HANDSHAKE_PAYLOAD_SIZE]);
        if (!data_) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        timeStart_ = HAL_Timer_Get_Milli_Seconds();
        return 0;
    }

    void destroy() {
        data_.reset();
        channel_->freeBuffer(buf_);
        buf_ = nullptr;
        size_ = 0;
        offs_ = 0;
    }

    int readPacket(const char** data, size_t* size) {
        if (HAL_Timer_Get_Milli_Seconds() - timeStart_ >= HANDSHAKE_TIMEOUT) {
            return SYSTEM_ERROR_TIMEOUT;
        }
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
        *data = data_.get();
        *size = size_;
        return 0;
    }

    void writePacket() {
        SPARK_ASSERT(buf_);
        // Serialize packet header
        HandshakeHeader h = {};
        h.size = nativeToLittleEndian(buf_->size);
        buf_->data -= HANDSHAKE_HEADER_SIZE;
        buf_->size += HANDSHAKE_HEADER_SIZE;
        memcpy(buf_->data, &h, HANDSHAKE_HEADER_SIZE);
        // Enqueue the buffer for sending
        channel_->sendBuffer(buf_);
        buf_ = nullptr;
    }

    int initPacket(Buffer** buf, size_t size) {
        SPARK_ASSERT(!buf_);
        CHECK(channel_->reallocBuffer(HANDSHAKE_HEADER_SIZE + size, &buf_));
        buf_->data += HANDSHAKE_HEADER_SIZE;
        buf_->size -= HANDSHAKE_HEADER_SIZE;
        *buf = buf_;
        return 0;
    }

    int initPacket(Buffer** buf) {
        return initPacket(buf, MAX_HANDSHAKE_PAYLOAD_SIZE);
    }

private:
    std::unique_ptr<char[]> data_;
    BleControlRequestChannel* channel_;
    BleControlRequestChannel::Buffer* buf_;
    system_tick_t timeStart_;
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
        memset(secret_, 0, sizeof(secret_));
        memset(confirmKey_, 0, sizeof(confirmKey_));
    }

    int init(const char* key, size_t keySize) {
        CHECK(HandshakeHandler::init());
        CHECK(hash_.init());
        mbedtls_ecjpake_init(&ctx_);
        CHECK_MBEDTLS(mbedtls_ecjpake_setup(&ctx_, MBEDTLS_ECJPAKE_SERVER, MBEDTLS_MD_SHA256, MBEDTLS_ECP_DP_SECP256R1,
                (const uint8_t*)key, keySize));
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
        case State::READ_CONFIRM:
            ret = readConfirm();
            break;
        case State::WRITE_CONFIRM:
            ret = writeConfirm();
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

    const char* secret() const {
        if (state_ != State::DONE) {
            return nullptr;
        }
        return secret_;
    }

private:
    enum class State {
        NEW,
        READ_ROUND1,
        WRITE_ROUND1,
        READ_ROUND2,
        WRITE_ROUND2,
        READ_CONFIRM,
        WRITE_CONFIRM,
        DONE,
        FAILED
    };

    Sha256 hash_;
    mbedtls_ecjpake_context ctx_;
    char secret_[JPAKE_SHARED_SECRET_SIZE];
    char confirmKey_[Sha256::SIZE];
    State state_;

    int readRound1() {
        const char* data = nullptr;
        size_t size = 0;
        const int ret = readPacket(&data, &size);
        if (ret != Result::DONE) {
            return ret;
        }
        CHECK_MBEDTLS(mbedtls_ecjpake_read_round_one(&ctx_, (const uint8_t*)data, size));
        CHECK(hash_.update(data, size));
        state_ = State::WRITE_ROUND1;
        return Result::RUNNING;
    }

    int writeRound1() {
        Buffer* buf = nullptr;
        CHECK(initPacket(&buf));
        size_t n = 0;
        CHECK_MBEDTLS(mbedtls_ecjpake_write_round_one(&ctx_, (uint8_t*)buf->data, buf->size, &n, mbedtls_default_rng, nullptr));
        buf->size = n;
        CHECK(hash_.update(buf->data, buf->size));
        writePacket();
        state_ = State::WRITE_ROUND2; // Send the round 2 message immediately
        return Result::RUNNING;
    }

    int readRound2() {
        const char* data = nullptr;
        size_t size = 0;
        const int ret = readPacket(&data, &size);
        if (ret != Result::DONE) {
            return ret;
        }
        CHECK_MBEDTLS(mbedtls_ecjpake_read_round_two(&ctx_, (const uint8_t*)data, size));
        CHECK(hash_.update(data, size));
        state_ = State::READ_CONFIRM;
        return Result::RUNNING;
    }

    int writeRound2() {
        Buffer* buf = nullptr;
        CHECK(initPacket(&buf));
        size_t n = 0;
        CHECK_MBEDTLS(mbedtls_ecjpake_write_round_two(&ctx_, (uint8_t*)buf->data, buf->size, &n, mbedtls_default_rng, nullptr));
        buf->size = n;
        CHECK(hash_.update(buf->data, buf->size));
        writePacket();
        state_ = State::READ_ROUND2;
        return Result::RUNNING;
    }

    int readConfirm() {
        const char* data = nullptr;
        size_t size = 0;
        const int ret = readPacket(&data, &size);
        if (ret != Result::DONE) {
            return ret;
        }
        if (size != Sha256::SIZE) {
            LOG_DEBUG(ERROR, "Invalid size of the confirmation message");
            return SYSTEM_ERROR_BAD_DATA;
        }
        // Derive the shared secret
        size_t n = 0;
        CHECK_MBEDTLS(mbedtls_ecjpake_derive_secret(&ctx_, (uint8_t*)secret_, sizeof(secret_), &n,
                mbedtls_default_rng, nullptr));
        if (n != JPAKE_SHARED_SECRET_SIZE) { // Sanity check
            LOG_DEBUG(ERROR, "Invalid size of the shared secret");
            return SYSTEM_ERROR_INTERNAL;
        }
        mbedtls_ecjpake_free(&ctx_);
        // Generate the confirmation key
        Sha256 hash;
        CHECK(hash.init());
        CHECK(hash.update(secret_, sizeof(secret_)));
        CHECK(hash.update("JPAKE_KC"));
        CHECK(hash.finish(confirmKey_));
        hash.destroy();
        // Validate the confirmation message
        CHECK(hash.init(hash_));
        char hashVal[Sha256::SIZE] = {};
        CHECK(hash.finish(hashVal));
        hash.destroy();
        HmacSha256 hmac;
        CHECK(hmac.init(confirmKey_, sizeof(confirmKey_)));
        CHECK(hmac.update("KC_1_U"));
        CHECK(hmac.update(JPAKE_CLIENT_ID));
        CHECK(hmac.update(JPAKE_SERVER_ID));
        CHECK(hmac.update(hashVal, sizeof(hashVal)));
        CHECK(hmac.finish(hashVal));
        if (memcmp(data, hashVal, Sha256::SIZE) != 0) {
            LOG_DEBUG(ERROR, "Invalid confirmation message");
            return SYSTEM_ERROR_BAD_DATA;
        }
        CHECK(hash_.update(data, size));
        HandshakeHandler::destroy(); // Free the buffer for received data
        state_ = State::WRITE_CONFIRM;
        return Result::RUNNING;
    }

    int writeConfirm() {
        Buffer* buf = nullptr;
        CHECK(initPacket(&buf, Sha256::SIZE));
        CHECK(hash_.finish(buf->data));
        hash_.destroy();
        HmacSha256 hmac;
        CHECK(hmac.init(confirmKey_, sizeof(confirmKey_)));
        CHECK(hmac.update("KC_1_U"));
        CHECK(hmac.update(JPAKE_SERVER_ID));
        CHECK(hmac.update(JPAKE_CLIENT_ID));
        CHECK(hmac.update(buf->data, buf->size));
        CHECK(hmac.finish(buf->data));
        writePacket();
        state_ = State::DONE;
        return Result::DONE;
    }
};

class BleControlRequestChannel::AesCcmCipher {
public:
    AesCcmCipher() :
            ctx_(),
            reqCount_(0),
            repCount_(0) {
    }

    ~AesCcmCipher() {
        mbedtls_ccm_free(&ctx_);
        memset(reqNonce_, 0, AES_CCM_FIXED_NONCE_SIZE);
        memset(repNonce_, 0, AES_CCM_FIXED_NONCE_SIZE);
    }

    int init(const char* key, const char* clientNonce, const char* serverNonce) {
        mbedtls_ccm_init(&ctx_);
        CHECK_MBEDTLS(mbedtls_ccm_setkey(&ctx_, MBEDTLS_CIPHER_ID_AES, (const uint8_t*)key, AES_CCM_KEY_SIZE * 8));
        memcpy(reqNonce_, clientNonce, AES_CCM_FIXED_NONCE_SIZE);
        memcpy(repNonce_, serverNonce, AES_CCM_FIXED_NONCE_SIZE);
        return 0;
    }

    int decryptRequestData(char* buf, size_t payloadSize) {
        char nonce[AES_CCM_NONCE_SIZE] = {};
        genRequestNonce(nonce);
        CHECK_MBEDTLS(mbedtls_ccm_auth_decrypt(&ctx_,
                payloadSize + REQUEST_HEADER_SIZE, // Size of the input data
                (const uint8_t*)nonce, AES_CCM_NONCE_SIZE, // Nonce
                (const uint8_t*)buf, MESSAGE_HEADER_SIZE, // Additional data
                (const uint8_t*)buf + MESSAGE_HEADER_SIZE, // Input buffer
                (uint8_t*)buf + MESSAGE_HEADER_SIZE, // Output buffer
                (const uint8_t*)buf + payloadSize + MESSAGE_HEADER_SIZE + REQUEST_HEADER_SIZE, AES_CCM_TAG_SIZE)); // Authentication tag
        return 0;
    }

    int encryptReplyData(char* buf, size_t payloadSize) {
        char nonce[AES_CCM_NONCE_SIZE] = {};
        genReplyNonce(nonce);
        CHECK_MBEDTLS(mbedtls_ccm_encrypt_and_tag(&ctx_,
                payloadSize + REPLY_HEADER_SIZE, // Size of the input data
                (const uint8_t*)nonce, AES_CCM_NONCE_SIZE, // Nonce
                (const uint8_t*)buf, MESSAGE_HEADER_SIZE, // Additional data
                (const uint8_t*)buf + MESSAGE_HEADER_SIZE, // Input buffer
                (uint8_t*)buf + MESSAGE_HEADER_SIZE, // Output buffer
                (uint8_t*)buf + payloadSize + MESSAGE_HEADER_SIZE + REPLY_HEADER_SIZE, AES_CCM_TAG_SIZE)); // Authentication tag
        return 0;
    }

private:
    mbedtls_ccm_context ctx_;
    char reqNonce_[AES_CCM_FIXED_NONCE_SIZE];
    char repNonce_[AES_CCM_FIXED_NONCE_SIZE];
    uint32_t reqCount_;
    uint32_t repCount_;

    void genRequestNonce(char* dest) {
        const uint32_t count = nativeToLittleEndian(++reqCount_);
        memcpy(dest, &count, 4);
        memcpy(dest + 4, reqNonce_, AES_CCM_FIXED_NONCE_SIZE);
    }

    void genReplyNonce(char* dest) {
        const uint32_t count = nativeToLittleEndian(++repCount_ | 0x80000000u);
        memcpy(dest, &count, 4);
        memcpy(dest + 4, repNonce_, AES_CCM_FIXED_NONCE_SIZE);
    }
};

BleControlRequestChannel::BleControlRequestChannel(ControlRequestHandler* handler) :
        ControlRequestChannel(handler),
#if BLE_CHANNEL_DEBUG_ENABLED
        allocReqCount_(0),
        heapBufCount_(0),
        poolBufCount_(0),
#endif
        inBufSize_(0),
        curReq_(nullptr),
        reqBufSize_(0),
        reqBufOffs_(0),
        packetSize_(0),
        connHandle_(BLE_INVALID_CONN_HANDLE),
        curConnHandle_(BLE_INVALID_CONN_HANDLE),
        connId_(0),
        curConnId_(0),
        packetCount_(0),
        maxPacketSize_(0),
        subscribed_(false),
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
#if BLE_CHANNEL_SECURITY_ENABLED
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
#endif
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
#if BLE_CHANNEL_SECURITY_ENABLED
        }
#endif
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
    CHECK(reallocBuffer(size + MESSAGE_HEADER_SIZE + REPLY_HEADER_SIZE + MESSAGE_FOOTER_SIZE, &req->repBuf));
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
    const auto req = static_cast<Request*>(ctrlReq);
    freeRequestData(req);
    req->result = result;
    req->handler = handler;
    req->handlerData = data;
    const std::lock_guard<Mutex> lock(readyReqsLock_);
    readyReqs_.pushBack(req);
}

int BleControlRequestChannel::initChannel() {
    packetBuf_.reset(new(std::nothrow) char[BLE_MAX_ATTR_VALUE_PACKET_SIZE]);
    if (!packetBuf_) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
#if BLE_CHANNEL_SECURITY_ENABLED
    CHECK(initJpake());
#endif
    return 0;
}

void BleControlRequestChannel::resetChannel() {
#if BLE_CHANNEL_SECURITY_ENABLED
    jpake_.reset();
    aesCcm_.reset();
#endif
    packetBuf_.reset();
    std::unique_lock<Mutex> lock(readyReqsLock_);
    while (Request* req = readyReqs_.popFront()) {
        freeRequest(req);
    }
    lock.unlock();
    while (Request* req = pendingReps_.popFront()) {
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
        CHECK(allocRequest(payloadSize, &curReq_));
        memcpy(curReq_->reqBuf, &mh, MESSAGE_HEADER_SIZE);
        reqBufSize_ = payloadSize + MESSAGE_HEADER_SIZE + REQUEST_HEADER_SIZE + MESSAGE_FOOTER_SIZE; // Total size of the request data
        reqBufOffs_ = MESSAGE_HEADER_SIZE;
    }
    // Read remaining request data
    const auto p = curReq_->reqBuf;
    const size_t n = readSome(p + reqBufOffs_, reqBufSize_ - reqBufOffs_);
    reqBufOffs_ += n;
    if (reqBufOffs_ < reqBufSize_) {
        return 0; // Wait for more data
    }
#if BLE_CHANNEL_SECURITY_ENABLED
    // Decrypt request data
    SPARK_ASSERT(aesCcm_);
    CHECK(aesCcm_->decryptRequestData(p, curReq_->request_size));
#endif
    // Parse request header
    RequestHeader rh = {};
    memcpy(&rh, p + MESSAGE_HEADER_SIZE, REQUEST_HEADER_SIZE);
    curReq_->id = littleEndianToNative(rh.id); // Request ID
    curReq_->type = littleEndianToNative(rh.type); // Request type
    LOG(TRACE, "Received a request message; type: %u, ID: %u", (unsigned)curReq_->type, (unsigned)curReq_->id);
    // Process request
    handler()->processRequest(curReq_, this);
    curReq_ = nullptr;
    reqBufSize_ = 0;
    reqBufOffs_ = 0;
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
    NAMED_SCOPE_GUARD(reqGuard, {
        freeRequest(req);
    });
    // Make sure we have a buffer to serialize the reply
    if (!req->repBuf) {
        CHECK(reallocBuffer(MESSAGE_HEADER_SIZE + REPLY_HEADER_SIZE + MESSAGE_FOOTER_SIZE, &req->repBuf));
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
#if BLE_CHANNEL_SECURITY_ENABLED
    // Encrypt reply data
    SPARK_ASSERT(aesCcm_);
    CHECK(aesCcm_->encryptReplyData(p, req->reply_size));
#endif
    // Enqueue the reply buffer for sending
    outBufs_.pushBack(req->repBuf);
    req->repBuf = nullptr;
    LOG(TRACE, "Enqueued a reply message for sending; ID: %u", (unsigned)req->id);
    if (req->handler) {
        pendingReps_.pushBack(req);
        reqGuard.dismiss();
    }
    return 0;
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
        if (packetCount_ == 0) {
            // Invoke completion handlers
            while (Request* req = pendingReps_.popFront()) {
                req->handler(SYSTEM_ERROR_NONE, req->handlerData);
                req->handler = nullptr;
                freeRequest(req);
            }
        }
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
    ++packetCount_;
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
        subscribed_ = charParam.notif_enabled;
    } else {
        subscribed_ = false;
    }
    writable_ = subscribed_;
    packetCount_ = 0;
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
    curConnHandle_ = BLE_INVALID_CONN_HANDLE;
    writable_ = false;
    // Update connection state counter
    curConnId_.fetch_add(1, std::memory_order_release);
    return 0;
}

int BleControlRequestChannel::connParamChanged(const ble_conn_param_changed_event_data& event) {
    ble_conn_param param = { .version = BLE_API_VERSION };
    CHECK(ble_get_conn_param(curConnHandle_, &param, nullptr));
    maxPacketSize_ = param.att_mtu_size - BLE_ATT_OPCODE_SIZE - BLE_ATT_HANDLE_SIZE;
    return 0;
}

int BleControlRequestChannel::charParamChanged(const ble_char_param_changed_event_data& event) {
    if (event.char_handle == sendCharHandle_) {
        ble_char_param param = { .version = BLE_API_VERSION };
        CHECK(ble_get_char_param(curConnHandle_, sendCharHandle_, &param, nullptr));
        subscribed_ = param.notif_enabled;
        writable_ = subscribed_;
    }
    return 0;
}

int BleControlRequestChannel::dataSent(const ble_data_sent_event_data& event) {
    if (subscribed_) {
        writable_ = true;
    }
    --packetCount_;
    return 0;
}

int BleControlRequestChannel::dataReceived(const ble_data_received_event_data& event) {
    if (event.char_handle == recvCharHandle_) {
        DEBUG("Received BLE packet");
        DEBUG_DUMP(event.data, event.size);
        Buffer* buf = nullptr;
        CHECK(allocPooledBuffer(event.size, &buf));
        memcpy(buf->data, event.data, event.size);
        inBufs_.pushBack(buf);
    }
    return 0;
}

int BleControlRequestChannel::initProfile() {
    // Register base UUID
    uint8_t uuidType = 0;
    CHECK(ble_add_base_uuid((const char*)BASE_UUID, &uuidType, nullptr));
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
    // RX
    ble_char& recvChar = chars[2];
    recvChar.uuid.type = uuidType;
    recvChar.uuid.uuid = RECV_CHAR_UUID;
    recvChar.type = BLE_CHAR_TYPE_RX;
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
    CHECK(get_device_name(devName, sizeof(devName)));
    LOG(INFO, "Device name: %s", devName);
    profile.device_name = devName;
    profile.services = &ctrlService;
    profile.service_count = 1;
    profile.manuf_data = &manufData;
    profile.callback = processBleEvent;
    profile.user_data = this;
    CHECK(ble_init_profile(&profile, nullptr));
    sendCharHandle_ = sendChar.handle;
    recvCharHandle_ = recvChar.handle;
    return 0;
}

#if BLE_CHANNEL_SECURITY_ENABLED
int BleControlRequestChannel::initAesCcm() {
    SPARK_ASSERT(jpake_);
    const auto secret = jpake_->secret();
    SPARK_ASSERT(secret);
    aesCcm_.reset(new(std::nothrow) AesCcmCipher);
    if (!aesCcm_) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    // First AES_CCM_KEY_SIZE bytes of the shared secret are used as the session key for AES-CCM
    // encryption, next two blocks of AES_CCM_FIXED_NONCE_SIZE bytes each are used as fixed parts
    // of client and server nonces respectively
    CHECK(aesCcm_->init(secret, secret + AES_CCM_KEY_SIZE, secret + AES_CCM_KEY_SIZE + AES_CCM_FIXED_NONCE_SIZE));
    return 0;
}

int BleControlRequestChannel::initJpake() {
    char secret[JPAKE_PASSPHRASE_SIZE] = {};
    static_assert(sizeof(secret) <= HAL_DEVICE_SECRET_SIZE, "");
    const int ret = hal_get_device_secret(secret, sizeof(secret), nullptr);
    CHECK(ret);
    if (ret < (int)sizeof(secret)) { // Sanity check
        LOG_DEBUG(ERROR, "Invalid size of the device secret data");
        return SYSTEM_ERROR_INTERNAL;
    }
    jpake_.reset(new(std::nothrow) JpakeHandler(this));
    if (!jpake_) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CHECK(jpake_->init(secret, sizeof(secret)));
    return 0;
}
#endif // BLE_CHANNEL_SECURITY_ENABLED

int BleControlRequestChannel::allocRequest(size_t size, Request** req) {
    std::unique_ptr<Request> r(new(std::nothrow) Request());
    if (!r) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    r->reqBuf = new(std::nothrow) char[size + MESSAGE_HEADER_SIZE + REQUEST_HEADER_SIZE + MESSAGE_FOOTER_SIZE];
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
#if BLE_CHANNEL_DEBUG_ENABLED
    const auto count = ++allocReqCount_;
    DEBUG("Allocated a request object (count: %u)", (unsigned)count);
#endif
    return 0;
}

void BleControlRequestChannel::freeRequest(Request* req) {
    if (!req) {
        return;
    }
    const auto handler = req->handler;
    const auto handlerData = req->handlerData;
    freeBuffer(req->repBuf);
    delete[] req->reqBuf;
    delete req;
#if BLE_CHANNEL_DEBUG_ENABLED
    const auto count = --allocReqCount_;
    DEBUG("Freed a request object (count: %u)", (unsigned)count);
#endif
    if (handler) {
        handler(SYSTEM_ERROR_UNKNOWN, handlerData);
    }
}

int BleControlRequestChannel::reallocBuffer(size_t size, Buffer** buf) {
    const auto b = reallocLinkedBuffer<Buffer>(*buf, size);
    if (!b) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    b->data = linkedBufferData(b);
    b->size = size;
#if BLE_CHANNEL_DEBUG_ENABLED
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
#if BLE_CHANNEL_DEBUG_ENABLED
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
#if BLE_CHANNEL_DEBUG_ENABLED
    const auto count = ++poolBufCount_;
    DEBUG("Allocated a pooled buffer, size: %u (count: %u)", (unsigned)size, (unsigned)count);
#endif
    return 0;
}

void BleControlRequestChannel::freePooledBuffer(Buffer* buf) {
    if (buf) {
        freeLinkedBuffer(buf, &pool_);
#if BLE_CHANNEL_DEBUG_ENABLED
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
