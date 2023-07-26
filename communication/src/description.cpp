/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#undef LOG_COMPILE_TIME_LEVEL

#include "description.h"

#include "protocol.h"
#include "message_channel.h"
#include "coap_message_encoder.h"
#include "coap_message_decoder.h"

#include "mbedtls_config.h"

#include <algorithm>
#include "enumclass.h"

#define CHECK_PROTOCOL(_expr) \
        do { \
            const auto _r = _expr; \
            if (_r != particle::protocol::ProtocolError::NO_ERROR) { \
                return _r; \
            } \
        } while (false)

namespace particle {

namespace protocol {

namespace {

// Maximum CoAP overhead per Describe request message:
//
// - Message header: 4 bytes;
// - Token: 1 byte;
// - Uri-Path (11): 2 bytes;
// - Uri-Query (15): 2 bytes;
// - Block1 (27): 4 bytes;
// - Payload marker: 1 byte.
const size_t MAX_REQUEST_COAP_OVERHEAD = 14;

// Maximum CoAP overhead per Describe response message:
//
// - Message header: 4 bytes;
// - Token: 1 byte;
// - ETag (4): 5 bytes;
// - Block2 (23): 5 bytes;
// - Payload marker: 1 byte.
const size_t MAX_RESPONSE_COAP_OVERHEAD = 16;

// Minimum and maximum supported block sizes for blockwise transfers
const size_t MIN_BLOCK_SIZE = 512;
const size_t MAX_BLOCK_SIZE = 1024;

#if PLATFORM_GEN >= 3
static_assert(MAX_BLOCK_SIZE + MAX_REQUEST_COAP_OVERHEAD <= PROTOCOL_BUFFER_SIZE &&
        MAX_BLOCK_SIZE + MAX_RESPONSE_COAP_OVERHEAD <= PROTOCOL_BUFFER_SIZE, "PROTOCOL_BUFFER_SIZE is too small");
#endif // PLATFORM_GEN >= 3

// Initial size of a dynamically allocated buffer for Describe data
const size_t INITIAL_BUFFER_SIZE = 256;

// An appender class that writes the data to a fixed size buffer first and then to a dynamically
// allocated buffer if the former has run out of space. The data from the fixed size buffer is
// copied to the dynamically allocated one
class BufferAppender2: public Appender {
public:
    using Appender::append;

    BufferAppender2(char* buf, size_t bufSize, Vector<char>* vec) :
            buf_(buf),
            bufOffs_(0),
            bufSize_(bufSize),
            dataSize_(0),
            vec_(vec),
            ok_(true) {
    }

    bool append(const uint8_t* data, size_t size) override {
        dataSize_ += size;
        if (!ok_) {
            return true; // This method never fails
        }
        if (buf_ && bufOffs_ < bufSize_) {
            // Write to the fixed size buffer
            const size_t n = std::min(size, bufSize_ - bufOffs_);
            memcpy(buf_ + bufOffs_, data, n);
            bufOffs_ += n;
            data += n;
            size -= n;
        }
        if (!size) {
            return true;
        }
        if (!vec_) {
            // Fixed size buffer is full but additional buffer was not provided
            ok_ = false;
            return true;
        }
        const size_t vecOffs = vec_->size();
        if (buf_ && bufSize_ > 0) {
            // Copy the data serialized so far to the additional buffer
            ok_ = resizeVector(vecOffs + bufSize_ + size);
            if (!ok_) {
                return true;
            }
            const auto vecData = vec_->data() + vecOffs;
            memcpy(vecData, buf_, bufSize_);
            memcpy(vecData + bufSize_, data, size);
            buf_ = nullptr;
            return true;
        }
        // Write to the additional buffer
        ok_ = resizeVector(vecOffs + size);
        if (!ok_) {
            return true;
        }
        memcpy(vec_->data() + vecOffs, data, size);
        return true;
    }

    size_t size() const {
        return dataSize_;
    }

    bool ok() const {
        return ok_;
    }

private:
    char* buf_;
    size_t bufOffs_;
    size_t bufSize_;
    size_t dataSize_;
    Vector<char>* vec_;
    bool ok_;

    bool resizeVector(size_t size) {
        if ((size_t)vec_->capacity() < size) {
            const size_t newCapacity = std::max(std::max((size_t)vec_->capacity() * 3 / 2, size), INITIAL_BUFFER_SIZE);
            if (!vec_->reserve(newCapacity)) {
                return false;
            }
        }
        vec_->resize(size); // Cannot fail
        return true;
    }
};

unsigned encodeBlockOption(unsigned num, unsigned size, bool m) {
    // RFC 7959, 2.2. Structure of a Block Option
    static_assert(MIN_BLOCK_SIZE == 512 && MAX_BLOCK_SIZE == 1024, "This code needs to be updated accordingly");
    SPARK_ASSERT(size == 1024 || size == 512);
    size = (size == 1024) ? 6 : 5; // SZX
    unsigned opt = (num << 4) | size;
    if (m) {
        opt |= 0x08;
    }
    return opt;
}

bool decodeBlockOption(unsigned opt, unsigned* num, unsigned* size, bool* m) {
    static_assert(MIN_BLOCK_SIZE == 512 && MAX_BLOCK_SIZE == 1024, "This code needs to be updated accordingly");
    switch (opt & 0x07) { // SZX
        case 6: *size = 1024; break;
        case 5: *size = 512; break;
        default: return false; // Unsupported block size
    }
    *num = opt >> 4;
    *m = opt & 0x08;
    return true;
}

void initDescribeRequest(CoapMessageEncoder* enc, token_t token, int flags) {
    enc->type(CoapType::CON);
    enc->code(CoapCode::POST);
    enc->id(0); // Encoded by the message channel
    enc->token((const char*)&token, sizeof(token));
    enc->option(CoapOption::URI_PATH, "d");
    // NOTE: option order is important (increasing ids)
    // FIXME: move type inference somewhere else instead of hardcoding here?
    if (flags & DescriptionType::DESCRIBE_SYSTEM) {
        unsigned contentFormat = to_underlying(CoapContentFormat::APPLICATION_OCTET_STREAM);
        enc->option(CoapOption::CONTENT_FORMAT, contentFormat);
    }
    const char uriQuery = flags;
    enc->option(CoapOption::URI_QUERY, &uriQuery, sizeof(uriQuery));
}

void initDescribeResponse(CoapMessageEncoder* enc, token_t token, int flags) {
    enc->type(CoapType::CON);
    enc->code(CoapCode::CONTENT);
    enc->id(0); // Encoded by the message channel
    enc->token((const char*)&token, sizeof(token));
    // NOTE: For DESCRIBE_SYSTEM Content-Type option is encoded in receiveRequest() and sendResponseBlock()
}

} // namespace

Description::Description(Protocol* proto) :
        proto_(proto),
        blockSize_(0),
        lastEtag_(0) {
}

Description::~Description() {
}

ProtocolError Description::sendRequest(int descFlags) {
    // As per RFC 7959, 2.5, an endpoint cannot perform multiple concurrent blockwise transfers to
    // the same resource. It's not known in advance (before serialization) whether a given Describe
    // request will require blockwise transfer, so we're queueing all further Describe requests if
    // there's a blockwise request that is being sent to the server already
    if (!activeReq_.has_value()) {
        CHECK_PROTOCOL(sendNextRequest(descFlags));
    } else if (!reqQueue_.contains(descFlags) && !reqQueue_.append(descFlags)) {
        return ProtocolError::NO_MEMORY;
    }
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::receiveRequest(const Message& msg) {
    CoapMessageDecoder dec;
    const int r = dec.decode((const char*)msg.buf(), msg.length());
    if (r < 0) {
        LOG(ERROR, "Failed to decode message: %d", r);
        return ProtocolError::MALFORMED_MESSAGE;
    }
    // Describe requests are required to be confirmable and have a token
    if (dec.type() != CoapType::CON) {
        LOG(WARN, "Unexpected request type: %u", (unsigned)dec.type());
        return sendErrorResponse(dec, CoapCode::BAD_REQUEST);
    }
    if (dec.tokenSize() != sizeof(token_t)) {
        LOG(WARN, "Unexpected token size: %u", (unsigned)dec.tokenSize());
        return sendErrorResponse(dec, CoapCode::BAD_REQUEST);
    }
    token_t reqToken = 0;
    memcpy(&reqToken, dec.token(), sizeof(reqToken));
    auto iter = dec.findOption(CoapOption::URI_QUERY);
    if (!iter) {
        LOG(WARN, "Invalid message options");
        return sendErrorResponse(dec, CoapCode::BAD_REQUEST);
    }
    // Get Describe type
    const int flags = iter.toUInt() & DescriptionType::DESCRIBE_MAX;
    if (flags != DescriptionType::DESCRIBE_METRICS && !(flags | DescriptionType::DESCRIBE_SYSTEM) &&
            !(flags | DescriptionType::DESCRIBE_APPLICATION)) {
        LOG(WARN, "Invalid message options");
        return sendErrorResponse(dec, CoapCode::BAD_OPTION);
    }
    size_t blockSize = 0;
    CHECK_PROTOCOL(getBlockSize(&blockSize));
    // Check if it's a blockwise request
    unsigned blockIndex = 0;
    iter = dec.findOption(CoapOption::BLOCK2);
    if (iter) {
        const auto blockOpt = iter.toUInt();
        unsigned recvBlockSize = 0;
        bool hasMore = false; // Ignored
        if (!decodeBlockOption(blockOpt, &blockIndex, &recvBlockSize, &hasMore)) {
            LOG(WARN, "Invalid message options");
            return sendErrorResponse(dec, CoapCode::BAD_OPTION);
        }
        if (recvBlockSize != blockSize) {
            // This is a deviation from RFC 7959 but we require the server, that knows the maximum
            // size of a CoAP message supported by the device, to always use the maximum supported
            // block size in its requests
            LOG(WARN, "Unexpected block size: %u", recvBlockSize);
            return sendErrorResponse(dec, CoapCode::BAD_OPTION);
        }
    }
    // Check if the requested Describe is being served already
    Response newResp = {};
    Response* resp = nullptr;
    int respIndex = 0;
    for (; respIndex < activeResps_.size(); ++respIndex) {
        if (activeResps_[respIndex].flags == flags) {
            resp = &activeResps_[respIndex];
            break;
        }
    }
    if ((!resp && blockIndex != 0) || (resp && blockIndex >= resp->blockCount)) {
        // Fail early as replying with new Describe data will anyway cause an ETag mismatch
        // error on the server
        LOG(WARN, "Unexpected block index: %u", blockIndex);
        return sendErrorResponse(dec, CoapCode::NOT_FOUND);
    }
    // Acknowledge the request
    CHECK_PROTOCOL(sendEmptyAck(dec.id()));
    // Prepare a response
    Message respMsg;
    CHECK_PROTOCOL(proto_->get_channel().create(respMsg));
    CoapMessageEncoder enc((char*)respMsg.buf(), respMsg.capacity());
    size_t payloadSize = 0;
    if (!resp) {
        initDescribeResponse(&enc, reqToken, flags);
        // FIXME: This is not ideal and needs to be refactored.
        // For normal responses we have to pre-encode Content-Format option here.
        // For blockwise responses this will happen for each block due to the requirement
        // of increasing order of option identifiers with ETag going before Content-Format.
        if (flags & DescriptionType::DESCRIBE_SYSTEM) {
            unsigned contentFormat = to_underlying(CoapContentFormat::APPLICATION_OCTET_STREAM);
            enc.option(CoapOption::CONTENT_FORMAT, contentFormat);
        }
        const size_t msgOffs = enc.payloadData() - (char*)respMsg.buf();
        Vector<char> buf;
        CHECK_PROTOCOL(getDescribeData(flags, &respMsg, msgOffs, &buf, &payloadSize));
        if (!buf.isEmpty()) {
            // Prepare a blockwise response
            if (flags & DescriptionType::DESCRIBE_SYSTEM) {
                // Re-encode response without Content-Type option
                respMsg.clear();
                enc = CoapMessageEncoder((char*)respMsg.buf(), respMsg.capacity());
                initDescribeResponse(&enc, reqToken, flags);
            }
            newResp.data = std::move(buf);
            newResp.blockCount = (newResp.data.size() + blockSize - 1) / blockSize;
            newResp.reqCount = 1;
            newResp.etag = ++lastEtag_;
            newResp.flags = flags;
            resp = &newResp;
        }
    } else {
        // The server always requests response blocks sequentially and never re-requests the blocks
        // that it has already received. It may however initiate a number of concurrent blockwise
        // requests to get the same Describe data. As an optimization, we count the number of such
        // concurrent requests based on whether the server has requested the first or the last block
        // of the Describe data, and free the Describe data when there's no more blockwise requests
        // accessing it.
        //
        // It's mentioned in the RFC that an endpoint has no way to perform multiple concurrent
        // blockwise requests to the same resource using the Block2 option alone, but it's possible
        // to do so given that concurrent requests use different tokens (see RFC 7959, 2.4)
        if (blockIndex == 0) {
            ++resp->reqCount;
        }
    }
    if (resp) {
        // Send a blockwise response
        CHECK_PROTOCOL(sendResponseBlock(*resp, &respMsg, reqToken, blockIndex));
        if (blockIndex == resp->blockCount - 1) {
            --resp->reqCount;
        }
        resp->lastAccessTime = millis();
    } else {
        // Send a regular response
        enc.payloadSize(payloadSize);
        CHECK_PROTOCOL(encodeAndSend(&enc, &respMsg));
    }
    if (!resp || blockIndex == resp->blockCount - 1) {
        // Sent a regular response or the last block of a blockwise response
        Ack ack = {};
        ack.msgId = respMsg.get_id();
        ack.flags = flags;
        if (!acks_.append(std::move(ack))) {
            return ProtocolError::NO_MEMORY;
        }
    }
    if (resp) {
        if (!resp->reqCount && respIndex < activeResps_.size()) {
            activeResps_.removeAt(respIndex);
        } else if (resp->reqCount && respIndex == activeResps_.size() && !activeResps_.append(std::move(newResp))) {
            return ProtocolError::NO_MEMORY;
        }
    }
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::receiveAckOrRst(const Message& msg, int* descFlags) {
    if (!activeReq_.has_value() && acks_.isEmpty()) {
        return ProtocolError::NO_ERROR;
    }
    CoapMessageDecoder dec;
    const int r = dec.decode((const char*)msg.buf(), msg.length());
    if (r < 0) {
        LOG(ERROR, "Failed to decode message: %d", r);
        return ProtocolError::MALFORMED_MESSAGE;
    }
    const bool isRst = (dec.type() == CoapType::RST);
    if (!isRst && dec.type() != CoapType::ACK) {
        LOG(ERROR, "Unexpected message type: %u", (unsigned)dec.type());
        return ProtocolError::INTERNAL;
    }
    const auto ackId = dec.id();
    if (activeReq_.has_value() && activeReq_->msgId == ackId) {
        if (isRst) {
            activeReq_.reset();
            return ProtocolError::MESSAGE_RESET;
        }
        if (!activeReq_->data.isEmpty()) {
            // Send the next block of the current blockwise request
            Message msg;
            CHECK_PROTOCOL(proto_->get_channel().create(msg));
            const auto token = proto_->get_next_token();
            CHECK_PROTOCOL(sendNextRequestBlock(&*activeReq_, &msg, token));
        } else {
            // Received an ACK for the last block of the current blockwise request
            const auto flags = activeReq_->flags;
            activeReq_.reset();
            if (!reqQueue_.isEmpty()) {
                CHECK_PROTOCOL(sendNextRequest(reqQueue_.takeFirst()));
            }
            *descFlags = flags;
        }
    } else {
        for (int i = 0; i < acks_.size(); ++i) {
            if (acks_[i].msgId == ackId) {
                // Received an ACK for a regular request, regular response or the last block of a
                // blockwise response
                const auto flags = acks_[i].flags;
                acks_.removeAt(i);
                if (isRst) {
                    return ProtocolError::MESSAGE_RESET;
                }
                *descFlags = flags;
                break;
            }
        }
    }
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::processTimeouts() {
    if (activeResps_.isEmpty()) {
        return ProtocolError::NO_ERROR;
    }
    const auto now = millis();
    int i = 0;
    do {
        const auto t = activeResps_.at(i).lastAccessTime;
        if (now - t >= COAP_BLOCKWISE_RESPONSE_TIMEOUT) {
            LOG(WARN, "Blockwise response timeout");
            activeResps_.removeAt(i);
        } else {
            ++i;
        }
    } while (i < activeResps_.size());
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::serialize(Appender* appender, int descFlags) {
    const auto& descriptor = proto_->get_descriptor();
    switch (descFlags) {
    case DescriptionType::DESCRIBE_METRICS: {
        if (!descriptor.append_metrics) {
            return ProtocolError::NOT_IMPLEMENTED;
        }
        appender->append((char)0); // Null byte means binary data
        // 16-bit Describe type
        appender->append((char)DescriptionType::DESCRIBE_METRICS);
        appender->append((char)0);
        const int flags = 1; // Use binary encoding
        const int page = 0; // Page number (unused)
        const bool ok = descriptor.append_metrics(Appender::callback, appender, flags, page, nullptr /* reserved */);
        if (!ok) {
            return ProtocolError::UNKNOWN;
        }
        break;
    }
    case DescriptionType::DESCRIBE_SYSTEM: {
        if (!descriptor.append_system_info) {
            return ProtocolError::NOT_IMPLEMENTED;
        }
        bool ok = descriptor.append_system_info(Appender::callback, appender, nullptr);
        if (!ok) {
            return ProtocolError::UNKNOWN;
        }
        break;
    }
    case DescriptionType::DESCRIBE_APPLICATION: {
        appender->append("{");
        if (descriptor.append_app_info) {
            // Use the new callback
            const bool ok = descriptor.append_app_info(Appender::callback, appender, nullptr /* reserved */);
            if (!ok) {
                return ProtocolError::UNKNOWN;
            }
        } else {
            // Use the compatibility callbacks
            // FIXME: This is no longer needed on Gen 2 and higher
            appender->append("\"f\":[");
            int n = descriptor.num_functions();
            for (int i = 0; i < n; ++i) {
                if (i) {
                    appender->append(',');
                }
                const auto key = descriptor.get_function_key(i);
                appender->append('"');
                appender->append((const uint8_t*)key, std::min(strlen(key), MAX_FUNCTION_KEY_LENGTH));
                appender->append('"');
            }
            appender->append("],\"v\":{");
            n = descriptor.num_variables();
            for (int i = 0; i < n; ++i) {
                if (i) {
                    appender->append(',');
                }
                const auto key = descriptor.get_variable_key(i);
                const auto type = descriptor.variable_type(key);
                appender->append('"');
                appender->append((const uint8_t*)key, std::min(strlen(key), MAX_VARIABLE_KEY_LENGTH));
                appender->append("\":");
                appender->append('0' + (char)type);
            }
            appender->append('}');
        }
        appender->append('}');
        break;
    }
    default:
        return ProtocolError::INTERNAL;
    }
    return ProtocolError::NO_ERROR;
}

void Description::reset() {
    activeReq_.reset();
    activeResps_.clear();
    reqQueue_.clear();
    acks_.clear();
    blockSize_ = 0;
}

ProtocolError Description::sendNextRequest(int flags) {
    SPARK_ASSERT(!activeReq_.has_value());
    // Serialize a Describe request
    Message msg;
    CHECK_PROTOCOL(proto_->get_channel().create(msg));
    CoapMessageEncoder enc((char*)msg.buf(), msg.capacity());
    const auto token = proto_->get_next_token();
    initDescribeRequest(&enc, token, flags);
    const size_t msgOffs = enc.payloadData() - (char*)msg.buf();
    Vector<char> buf;
    size_t payloadSize = 0;
    CHECK_PROTOCOL(getDescribeData(flags, &msg, msgOffs, &buf, &payloadSize));
    if (!buf.isEmpty()) {
        // Send a blockwise request
        Request req = {};
        req.data = std::move(buf);
        req.flags = flags;
        CHECK_PROTOCOL(sendNextRequestBlock(&req, &msg, token));
        SPARK_ASSERT(!req.data.isEmpty());
        activeReq_ = std::move(req);
    } else {
        // Send a regular request
        enc.payloadSize(payloadSize);
        CHECK_PROTOCOL(encodeAndSend(&enc, &msg));
        Ack ack = {};
        ack.msgId = msg.get_id();
        ack.flags = flags;
        if (!acks_.append(std::move(ack))) {
            return ProtocolError::NO_MEMORY;
        }
    }
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::sendNextRequestBlock(Request* req, Message* msg, token_t token) {
    CoapMessageEncoder enc((char*)msg->buf(), msg->capacity());
    initDescribeRequest(&enc, token, req->flags);
    size_t blockSize = 0;
    CHECK_PROTOCOL(getBlockSize(&blockSize));
    size_t payloadSize = req->data.size();
    bool hasMore = false;
    if (payloadSize > blockSize) {
        payloadSize = blockSize;
        hasMore = true;
    }
    const auto blockOpt = encodeBlockOption(req->nextBlockIndex, blockSize, hasMore);
    enc.option(CoapOption::BLOCK1, blockOpt);
    enc.payload(req->data.data(), payloadSize);
    CHECK_PROTOCOL(encodeAndSend(&enc, msg));
    req->msgId = msg->get_id();
    const size_t newSize = req->data.size() - payloadSize;
    memmove(req->data.data(), req->data.data() + payloadSize, newSize);
    req->data.resize(newSize);
    req->data.trimToSize(); // Ignore error
    ++req->nextBlockIndex;
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::sendResponseBlock(const Response& resp, Message* msg, token_t token, unsigned blockIndex) {
    CoapMessageEncoder enc((char*)msg->buf(), msg->capacity());
    initDescribeResponse(&enc, token, resp.flags);
    enc.option(CoapOption::ETAG, resp.etag);
    size_t blockSize = 0;
    CHECK_PROTOCOL(getBlockSize(&blockSize));
    size_t offs = blockIndex * blockSize;
    size_t payloadSize = resp.data.size() - offs;
    bool hasMore = false;
    if (payloadSize > blockSize) {
        payloadSize = blockSize;
        hasMore = true;
    }
    if (resp.flags & DescriptionType::DESCRIBE_SYSTEM) {
        unsigned contentFormat = to_underlying(CoapContentFormat::APPLICATION_OCTET_STREAM);
        enc.option(CoapOption::CONTENT_FORMAT, contentFormat);
    }
    const auto blockOpt = encodeBlockOption(blockIndex, blockSize, hasMore);
    enc.option(CoapOption::BLOCK2, blockOpt);
    enc.payload(resp.data.data() + offs, payloadSize);
    CHECK_PROTOCOL(encodeAndSend(&enc, msg));
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::sendErrorResponse(const CoapMessageDecoder& reqDec, CoapCode code) {
    const bool isCon = (reqDec.type() == CoapType::CON);
    if (!isCon && reqDec.type() != CoapType::NON) {
        return ProtocolError::INTERNAL;
    }
    Message msg;
    CHECK_PROTOCOL(proto_->get_channel().create(msg));
    CoapMessageEncoder enc((char*)msg.buf(), msg.capacity());
    enc.type(isCon ? CoapType::ACK : CoapType::NON);
    enc.code(code);
    enc.id(0); // Encoded by the message channel
    enc.token(reqDec.token(), reqDec.tokenSize());
    if (isCon) {
        msg.set_id(reqDec.id());
    }
    CHECK_PROTOCOL(encodeAndSend(&enc, &msg));
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::sendEmptyAck(message_id_t msgId) {
    Message msg;
    CHECK_PROTOCOL(proto_->get_channel().create(msg));
    CoapMessageEncoder enc((char*)msg.buf(), msg.capacity());
    enc.type(CoapType::ACK);
    enc.code(CoapCode::EMPTY);
    enc.id(0); // Encoded by the message channel
    msg.set_id(msgId);
    CHECK_PROTOCOL(encodeAndSend(&enc, &msg));
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::encodeAndSend(CoapMessageEncoder* enc, Message* msg) {
    const size_t maxMsgSize = proto_->get_max_transmit_message_size();
    const int r = enc->encode();
    if (r < 0 || r > (int)maxMsgSize) {
        LOG(ERROR, "Failed to encode message: %d", r);
        return ProtocolError::INTERNAL;
    }
    msg->set_length(r);
    CHECK_PROTOCOL(proto_->get_channel().send(*msg));
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::getDescribeData(int flags, Message* msg, size_t msgOffs, Vector<char>* buf, size_t* size) {
    const size_t maxMsgSize = proto_->get_max_transmit_message_size();
    SPARK_ASSERT(msgOffs <= maxMsgSize);
    BufferAppender2 appender((char*)msg->buf() + msgOffs, maxMsgSize - msgOffs, buf);
    CHECK_PROTOCOL(serialize(&appender, flags));
    if (!appender.ok()) {
        return ProtocolError::NO_MEMORY;
    }
    *size = appender.size();
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::getBlockSize(size_t* size) {
    static_assert(MIN_BLOCK_SIZE == 512 && MAX_BLOCK_SIZE == 1024, "This code needs to be updated accordingly");
    if (!blockSize_) {
        const size_t maxMsgSize = proto_->get_max_transmit_message_size();
        const size_t coapOverhead = std::max(MAX_REQUEST_COAP_OVERHEAD, MAX_RESPONSE_COAP_OVERHEAD);
        if (coapOverhead + 1024 <= maxMsgSize) {
            blockSize_ = 1024;
        } else if (coapOverhead + 512 <= maxMsgSize) {
            blockSize_ = 512;
        } else {
            LOG(ERROR, "Failed to determine block size");
            return ProtocolError::INTERNAL;
        }
    }
    *size = blockSize_;
    return ProtocolError::NO_ERROR;
}

system_tick_t Description::millis() const {
    return proto_->get_callbacks().millis();
}

} // namespace protocol

} // namespace particle
