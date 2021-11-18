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

#include <algorithm>

namespace particle {

namespace protocol {

namespace {

#if PLATFORM_GEN >= 3
const size_t BLOCK_SIZE = 1024;
#else
const size_t BLOCK_SIZE = 512;
#endif

/**
 * Maximum CoAP overhead per Describe request message:
 *
 * - Message header: 4 bytes;
 * - Token: 1 bytes;
 * - Uri-Path (11): 2 bytes;
 * - Uri-Query (15): 2 bytes;
 * - Block1 (27): 4 bytes;
 * - Payload marker: 1 byte.
 */
const size_t REQUEST_COAP_OVERHEAD = 14;

/**
 * Maximum CoAP overhead per Describe response message:
 *
 * - Message header: 4 bytes;
 * - Token: 1 bytes;
 * - ETag (4): 5 bytes;
 * - Block2 (23): 5 bytes;
 * - Payload marker: 1 byte.
 */
const size_t RESPONSE_COAP_OVERHEAD = 16;

const unsigned RESPONSE_TIMEOUT = 90000;

static_assert(BLOCK_SIZE + REQUEST_COAP_OVERHEAD <= MBEDTLS_SSL_MAX_CONTENT_LEN &&
        BLOCK_SIZE + RESPONSE_COAP_OVERHEAD <= MBEDTLS_SSL_MAX_CONTENT_LEN, "BLOCK_SIZE is too large");

class CharVectorAppender: public Appender {
public:
    using Appender::append;

    explicit CharVectorAppender(Vector<char>* vec) :
            vec_(vec),
            ok_(true) {
    }

    bool append(const uint8_t* data, size_t size) override {
        if (!ok_) {
            return false;
        }
        const size_t offs = vec_->size();
        if (vec_->capacity() - offs < size) {
            const size_t n = std::max(std::max((size_t)vec_->capacity() * 3 / 2, offs + size), INITIAL_CAPACITY);
            ok_ = vec_->reserve(n);
            if (!ok_) {
                return false;
            }
        }
        vec_->resize(offs + size);
        memcpy(vec_->data() + offs, data, size);
        return true;
    }

    bool isOk() const {
        return ok_;
    }

private:
    Vector<char>* vec_;
    bool ok_;

    static const size_t INITIAL_CAPACITY = 256;
};

unsigned encodeBlockOption(unsigned num, bool m) {
    // RFC 7959, 2.2. Structure of a Block Option
    static_assert(BLOCK_SIZE == 1024 || BLOCK_SIZE == 512, "Unsupported BLOCK_SIZE");
    const unsigned szx = (BLOCK_SIZE == 1024) ? 6 /* 1024 bytes */ : 5 /* 512 bytes */;
    unsigned blockOpt = (num << 4) | szx;
    if (m) {
        blockOpt |= 0x08;
    }
    return blockOpt;
}

bool decodeBlockOption(unsigned blockOpt, unsigned* num, bool* m) {
    static_assert(BLOCK_SIZE == 1024 || BLOCK_SIZE == 512, "Unsupported BLOCK_SIZE");
    const unsigned szx = blockOpt & 0x07;
    if (!((BLOCK_SIZE == 1024 && szx == 6 /* 1024 bytes */) || (BLOCK_SIZE == 512 && szx == 5 /* 512 bytes */))) {
        return false; // Unsupported block size
    }
    *num = blockOpt >> 4;
    *m = blockOpt & 0x08;
    return true;
}

} // namespace

Description::Description(Protocol* proto) :
        proto_(proto),
        lastEtag_(0) {
}

Description::~Description() {
}

ProtocolError Description::sendRequest(int flags) {
    Request req = {};
    req.flags = flags;
    ProtocolError err = getDescribeData(&req.data, flags);
    if (err != ProtocolError::NO_ERROR) {
        return err;
    }
    bool hasMore = false;
    err = sendNextRequestBlock(&req, &hasMore);
    if (err != ProtocolError::NO_ERROR) {
        return err;
    }
    if (!hasMore) {
        Ack ack = {};
        ack.msgId = req.msgId;
        ack.flags = flags;
        if (!acks_.append(std::move(ack))) {
            return ProtocolError::NO_MEMORY;
        }
    } else if (!reqs_.append(std::move(req))) {
        return ProtocolError::NO_MEMORY;
    }
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::processRequest(const Message& msg) {
    CoapMessageDecoder dec;
    const int r = dec.decode((const char*)msg.buf(), msg.length());
    if (r < 0) {
        LOG(ERROR, "Failed to decode message: %d", r);
        return ProtocolError::MALFORMED_MESSAGE;
    }
    // Describe requests are required to be confirmable and have a token
    if (dec.type() != CoapType::CON) {
        LOG(WARN, "Unexpected message type: %u", (unsigned)dec.type());
        return sendErrorResponse(dec, CoapCode::BAD_REQUEST);
    }
    if (dec.tokenSize() != sizeof(token_t)) {
        LOG(WARN, "Unexpected token size: %u", (unsigned)dec.tokenSize());
        return sendErrorResponse(dec, CoapCode::BAD_REQUEST);
    }
    token_t token = 0;
    memcpy(&token, dec.token(), sizeof(token));
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
    // Check if it's a blockwise request
    unsigned blockIndex = 0;
    iter = dec.findOption(CoapOption::BLOCK2);
    if (iter) {
        const auto blockOpt = iter.toUInt();
        bool hasMore = false;
        if (!decodeBlockOption(blockOpt, &blockIndex, &hasMore)) {
            LOG(WARN, "Invalid message options");
            return sendErrorResponse(dec, CoapCode::BAD_OPTION);
        }
    }
    // Check if the requested Describe is being served already
    Response newResp = {};
    Response* resp = nullptr;
    int respIndex = 0;
    for (; respIndex < resps_.size(); ++respIndex) {
        if (resps_[respIndex].flags == flags) {
            resp = &resps_[respIndex];
            break;
        }
    }
    if (!resp) {
        if (blockIndex != 0) {
            // Fail early as replying with a block of a newly serialized Describe data will cause
            // a ETag mismatch error on the server
            LOG(WARN, "Unexpected block index: %u", blockIndex);
            return sendErrorResponse(dec, CoapCode::NOT_FOUND);
        }
        const ProtocolError err = getDescribeData(&newResp.data, flags);
        if (err != ProtocolError::NO_ERROR) {
            return err;
        }
        newResp.flags = flags;
        newResp.etag = ++lastEtag_;
        resp = &newResp;
    } else if (blockIndex >= resp->blockCount) {
        LOG(WARN, "Unexpected block index: %u", blockIndex);
        return sendErrorResponse(dec, CoapCode::NOT_FOUND);
    }
    // Acknowledge the request
    ProtocolError err = sendEmptyAck(dec);
    if (err != ProtocolError::NO_ERROR) {
        return err;
    }
    // Send a response
    message_id_t respMsgId = 0;
    bool hasMore = false;
    err = sendResponseBlock(*resp, token, blockIndex, &respMsgId, &hasMore);
    if (err != ProtocolError::NO_ERROR) {
        return err;
    }
    if (hasMore) {
        if (respIndex >= resps_.size()) {
            newResp.blockCount = (newResp.data.size() + BLOCK_SIZE - 1) / BLOCK_SIZE;
            if (!resps_.append(std::move(newResp))) {
                return ProtocolError::NO_MEMORY;
            }
            resp = &resps_.last();
            respIndex = resps_.size() - 1;
        }
        if (blockIndex == 0) {
            ++resp->reqCount;
        }
        resp->accessTime = millis();
    } else {
        if (respIndex < resps_.size()) {
            if (--resp->reqCount == 0) {
                resps_.removeAt(respIndex);
            } else {
                resp->accessTime = millis();
            }
        }
        Ack ack = {};
        ack.msgId = respMsgId;
        ack.flags = flags;
        if (!acks_.append(std::move(ack))) {
            return ProtocolError::NO_MEMORY;
        }
    }
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::processAck(const Message& msg, int* flags) {
    if (reqs_.isEmpty() && acks_.isEmpty()) {
        return ProtocolError::NO_ERROR;
    }
    CoapMessageDecoder dec;
    const int r = dec.decode((const char*)msg.buf(), msg.length());
    if (r < 0) {
        LOG(ERROR, "Failed to decode message: %d", r);
        return ProtocolError::MALFORMED_MESSAGE;
    }
    for (int i = 0; i < reqs_.size(); ++i) {
        auto& req = reqs_.at(i);
        if (req.msgId == dec.id()) {
            if (dec.type() == CoapType::ACK) {
                // Send next block
                bool hasMore = false;
                const ProtocolError err = sendNextRequestBlock(&req, &hasMore);
                if (err != ProtocolError::NO_ERROR) {
                    return err;
                }
                if (!hasMore) {
                    Ack ack = {};
                    ack.msgId = req.msgId;
                    ack.flags = req.flags;
                    if (!acks_.append(std::move(ack))) {
                        return ProtocolError::NO_MEMORY;
                    }
                    reqs_.removeAt(i);
                }
            } else {
                reqs_.removeAt(i);
            }
            return ProtocolError::NO_ERROR;
        }
    }
    for (int i = 0; i < acks_.size(); ++i) {
        if (acks_[i].msgId == dec.id()) {
            if (dec.type() == CoapType::ACK) {
                *flags = acks_[i].flags;
            }
            acks_.removeAt(i);
            return ProtocolError::NO_ERROR;
        }
    }
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::processTimeouts() {
    if (resps_.isEmpty()) {
        return ProtocolError::NO_ERROR;
    }
    const auto now = millis();
    int i = 0;
    do {
        const auto& resp = resps_.at(i);
        if (now - resp.accessTime >= RESPONSE_TIMEOUT) {
            LOG(WARN, "Blockwise response timeout");
            resps_.removeAt(i);
        } else {
            ++i;
        }
    } while (i < resps_.size());
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::serialize(Appender* appender, int flags) {
    const auto& descriptor = proto_->getDescriptor();
    if (flags == DescriptionType::DESCRIBE_METRICS) {
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
    } else {
        if (!(flags & (DescriptionType::DESCRIBE_APPLICATION | DescriptionType::DESCRIBE_SYSTEM))) {
            return ProtocolError::INTERNAL;
        }
        bool hasContent = false;
        appender->append("{");
        if (flags & DescriptionType::DESCRIBE_APPLICATION) {
            if (descriptor.append_app_info) {
                // Use the new callback
                const bool ok = descriptor.append_app_info(Appender::callback, appender, nullptr /* reserved */);
                if (!ok) {
                    return ProtocolError::UNKNOWN;
                }
            } else {
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
            hasContent = true;
        }
        if (flags & DescriptionType::DESCRIBE_SYSTEM) {
            if (!descriptor.append_system_info) {
                return ProtocolError::NOT_IMPLEMENTED;
            }
            if (hasContent) {
                appender->append(',');
            }
            const bool ok = descriptor.append_system_info(Appender::callback, appender, nullptr);
            if (!ok) {
                return ProtocolError::UNKNOWN;
            }
            hasContent = true;
        }
        appender->append('}');
    }
    return ProtocolError::NO_ERROR;
}

void Description::reset() {
    reqs_.clear();
    resps_.clear();
    acks_.clear();
}

// TODO: Decouple blockwise transfer from the Describe handling logic
ProtocolError Description::sendNextRequestBlock(Request* req, bool* hasMoreArg) {
    const auto channel = &proto_->getChannel();
    Message msg;
    ProtocolError err = channel->create(msg);
    if (err != ProtocolError::NO_ERROR) {
        LOG(ERROR, "Failed to create message");
        return err;
    }
    CoapMessageEncoder enc((char*)msg.buf(), msg.capacity());
    enc.type(CoapType::CON);
    enc.code(CoapCode::POST);
    enc.id(0); // Encoded by the message channel
    const auto token = proto_->get_next_token();
    enc.token((const char*)&token, sizeof(token));
    enc.option(CoapOption::URI_PATH, "d");
    const char uriQuery = req->flags;
    enc.option(CoapOption::URI_QUERY, &uriQuery, sizeof(uriQuery));
    bool hasMore = false;
    size_t offs = req->blockIndex * BLOCK_SIZE;
    size_t payloadSize = req->data.size() - offs;
    if (payloadSize > enc.maxPayloadSize() || req->blockIndex > 0) {
        if (payloadSize > BLOCK_SIZE) {
            payloadSize = BLOCK_SIZE;
        }
        hasMore = offs + payloadSize < (size_t)req->data.size();
        const auto blockOpt = encodeBlockOption(req->blockIndex, hasMore);
        enc.option(CoapOption::BLOCK1, blockOpt);
    }
    enc.payload(req->data.data() + offs, payloadSize);
    const int r = enc.encode();
    if (r < 0 || r > (int)msg.capacity()) {
        LOG(ERROR, "Failed to encode message: %d", r);
        return ProtocolError::INTERNAL;
    }
    msg.set_length(r);
    err = channel->send(msg);
    if (err != ProtocolError::NO_ERROR) {
        LOG(ERROR, "Failed to send message: %d", (int)err);
        return err;
    }
    req->msgId = msg.get_id();
    ++req->blockIndex;
    *hasMoreArg = hasMore;
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::sendResponseBlock(const Response& resp, token_t token, unsigned blockIndex, message_id_t* msgId,
        bool* hasMoreArg) {
    const auto channel = &proto_->getChannel();
    Message msg;
    ProtocolError err = channel->create(msg);
    if (err != ProtocolError::NO_ERROR) {
        LOG(ERROR, "Failed to create message");
        return err;
    }
    CoapMessageEncoder enc((char*)msg.buf(), msg.capacity());
    enc.type(CoapType::CON);
    enc.code(CoapCode::CONTENT);
    enc.id(0); // Encoded by the message channel
    enc.token((const char*)&token, sizeof(token));
    bool hasMore = false;
    size_t offs = blockIndex * BLOCK_SIZE;
    size_t payloadSize = resp.data.size() - offs;
    if (payloadSize > enc.maxPayloadSize() || blockIndex > 0) {
        // RFC 7959, 2.4: Block-wise transfers can be used to GET resources whose representations
        // are entirely static (not changing over time at all ...), or for dynamically changing
        // resources. In the latter case, the Block2 Option SHOULD be used in conjunction with the
        // ETag Option
        enc.option(CoapOption::ETAG, resp.etag);
        if (payloadSize > BLOCK_SIZE) {
            payloadSize = BLOCK_SIZE;
        }
        hasMore = offs + payloadSize < (size_t)resp.data.size();
        const auto blockOpt = encodeBlockOption(blockIndex, hasMore);
        enc.option(CoapOption::BLOCK2, blockOpt);
    }
    enc.payload(resp.data.data() + offs, payloadSize);
    const int r = enc.encode();
    if (r < 0 || r > (int)msg.capacity()) {
        LOG(ERROR, "Failed to encode message: %d", r);
        return ProtocolError::INTERNAL;
    }
    msg.set_length(r);
    err = channel->send(msg);
    if (err != ProtocolError::NO_ERROR) {
        LOG(ERROR, "Failed to send message: %d", (int)err);
        return err;
    }
    *msgId = msg.get_id();
    *hasMoreArg = hasMore;
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::sendErrorResponse(const CoapMessageDecoder& reqDec, CoapCode code) {
    const auto channel = &proto_->getChannel();
    Message msg;
    ProtocolError err = channel->create(msg);
    if (err != ProtocolError::NO_ERROR) {
        LOG(ERROR, "Failed to create message");
        return err;
    }
    CoapMessageEncoder enc((char*)msg.buf(), msg.capacity());
    enc.type((reqDec.type() == CoapType::CON) ? CoapType::ACK : CoapType::NON);
    enc.code(code);
    enc.id(0); // Encoded by the message channel
    enc.token(reqDec.token(), reqDec.tokenSize());
    const int r = enc.encode();
    if (r < 0 || r > (int)msg.capacity()) {
        LOG(ERROR, "Failed to encode message: %d", r);
        return ProtocolError::INTERNAL;
    }
    msg.set_length(r);
    if (reqDec.type() == CoapType::CON) {
        msg.set_id(reqDec.id());
    }
    err = channel->send(msg);
    if (err != ProtocolError::NO_ERROR) {
        LOG(ERROR, "Failed to send message: %d", (int)err);
        return err;
    }
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::sendEmptyAck(const CoapMessageDecoder& reqDec) {
    const auto channel = &proto_->getChannel();
    Message msg;
    ProtocolError err = channel->create(msg);
    if (err != ProtocolError::NO_ERROR) {
        LOG(ERROR, "Failed to create message");
        return err;
    }
    CoapMessageEncoder enc((char*)msg.buf(), msg.capacity());
    enc.type(CoapType::ACK);
    enc.code(CoapCode::EMPTY);
    enc.id(0); // Encoded by the message channel
    const int r = enc.encode();
    if (r < 0 || r > (int)msg.capacity()) {
        LOG(ERROR, "Failed to encode message: %d", r);
        return ProtocolError::INTERNAL;
    }
    msg.set_length(r);
    msg.set_id(reqDec.id());
    err = channel->send(msg);
    if (err != ProtocolError::NO_ERROR) {
        LOG(ERROR, "Failed to send message: %d", (int)err);
        return err;
    }
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::getDescribeData(Vector<char>* data, int flags) {
    CharVectorAppender appender(data);
    const ProtocolError err = serialize(&appender, flags);
    if (err != ProtocolError::NO_ERROR) {
        LOG(ERROR, "Failed to serialize Describe data: %d", (int)err);
        return err;
    }
    if (!appender.isOk()) {
        return ProtocolError::NO_MEMORY;
    }
    return ProtocolError::NO_ERROR;
}

system_tick_t Description::millis() const {
    return proto_->get_callbacks().millis();
}

} // namespace protocol

} // namespace particle
