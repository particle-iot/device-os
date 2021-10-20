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

namespace particle {

namespace protocol {

namespace {

const size_t BUFFER_SIZE_INCREMENT = 256;
const size_t MAX_BLOCK_SIZE = 1024;

/**
 * Maximum CoAP overhead per Describe message:
 *
 * - Message header: 4 bytes;
 * - Token: 1 byte;
 * - Uri-Path: 2 bytes;
 * - Uri-Query: 2 bytes;
 * - Block1: 6 bytes;
 * - Size1: 6 bytes;
 * - Payload marker: 1 byte.
 */
const size_t DESCRIBE_COAP_OVERHEAD = 22;

static_assert(MBEDTLS_SSL_MAX_CONTENT_LEN >= MAX_BLOCK_SIZE + DESCRIBE_COAP_OVERHEAD,
        "MBEDTLS_SSL_MAX_CONTENT_LEN is too small");

} // namespace

struct Description::DescribeMessage {
    char* buf; // Description data
    size_t bufSize; // Buffer size
    size_t bufOffs; // Current offset in the buffer
    unsigned blockIndex; // CoAP block index
    int type; // Description type
    token_t token; // CoAP message token
    bool isRequest; // Whether this message is a request or response

    explicit DescribeMessage(int type) :
            buf(nullptr),
            bufSize(0),
            bufOffs(0),
            blockIndex(0),
            type(type),
            token(0),
            isRequest(false) {
    }

    ~DescribeMessage() {
        free(buf);
    }
};

Description::Description(Protocol* proto) :
        proto_(proto) {
}

Description::~Description() {
}

ProtocolError Description::beginRequest(int type) {
    if (curMsg_) {
        return ProtocolError::INVALID_STATE;
    }
    std::unique_ptr<DescribeMessage> msg(new(std::nothrow) DescribeMessage(type));
    if (!msg) {
        return ProtocolError::NO_MEMORY;
    }
    msg->isRequest = true;
    curMsg_ = std::move(msg);
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::beginResponse(int type, token_t token) {
    if (curMsg_) {
        return ProtocolError::INVALID_STATE;
    }
    std::unique_ptr<DescribeMessage> msg(new(std::nothrow) DescribeMessage(type));
    if (!msg) {
        return ProtocolError::NO_MEMORY;
    }
    msg->token = token;
    msg->isRequest = false;
    curMsg_ = std::move(msg);
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::write(const char* data, size_t size) {
    if (!curMsg_) {
        return ProtocolError::INVALID_STATE;
    }
    const auto bufAvail = curMsg_->bufSize - curMsg_->bufOffs;
    if (bufAvail < size) {
        const auto bufSize = (curMsg_->bufSize + size - bufAvail + BUFFER_SIZE_INCREMENT - 1) / BUFFER_SIZE_INCREMENT *
                BUFFER_SIZE_INCREMENT;
        const auto buf = (char*)realloc(curMsg_->buf, bufSize);
        if (!buf) {
            return ProtocolError::NO_MEMORY;
        }
        curMsg_->buf = buf;
        curMsg_->bufSize = bufSize;
    }
    memcpy(curMsg_->buf + curMsg_->bufOffs, data, size);
    curMsg_->bufOffs += size;
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::send() {
    if (!curMsg_) {
        return ProtocolError::INVALID_STATE;
    }
    std::unique_ptr<DescribeMessage> msg;
    msg.swap(curMsg_);
    if (msg->isRequest) {
        msg->token = proto_->get_next_token();
    }
    msg->bufSize = msg->bufOffs; // Actual size of the description data
    msg->bufOffs = 0;
    const auto r = sendNext(msg.get());
    if (r != ProtocolError::NO_ERROR) {
        return r;
    }
    if (msg->bufOffs < msg->bufSize && !msgs_.append(std::move(msg))) {
        return ProtocolError::NO_MEMORY;
    }
    return ProtocolError::NO_ERROR;
}

void Description::cancel() {
    curMsg_.reset();
}

ProtocolError Description::processAck(const Message* coapMsg, int* type) {
    if (msgs_.isEmpty()) {
        return ProtocolError::NO_ERROR;
    }
    CoapMessageDecoder dec;
    const int r = dec.decode((const char*)coapMsg->buf(), coapMsg->length());
    if (r < 0) {
        return ProtocolError::MALFORMED_MESSAGE;
    }
    if (dec.tokenSize() != sizeof(token_t)) {
        return ProtocolError::NO_ERROR;
    }
    token_t token = 0;
    memcpy(&token, dec.token(), dec.tokenSize());
    DescribeMessage* msg = nullptr;
    size_t msgIndex = 0;
    for (int i = 0; i < msgs_.size(); ++i) {
        if (msgs_[i]->token == token) {
            msg = msgs_[i].get();
            msgIndex = i;
            break;
        }
    }
    int t = DescriptionType::DESCRIBE_NONE;
    if (msg) {
        const auto r = sendNext(msg);
        if (r != ProtocolError::NO_ERROR) {
            msgs_.removeAt(msgIndex);
            return r;
        }
        if (msg->bufOffs >= msg->bufSize) {
            t = msg->type;
            msgs_.removeAt(msgIndex);
        }
    }
    if (type) {
        *type = t;
    }
    return ProtocolError::NO_ERROR;
}

ProtocolError Description::processTimeouts() {
    return ProtocolError::NO_ERROR; // TODO
}

void Description::reset() {
    msgs_.clear();
    curMsg_.reset();
}

ProtocolError Description::sendNext(DescribeMessage* msg) {
    const auto channel = &proto_->getChannel();
    Message coapMsg;
    ProtocolError r = channel->create(coapMsg);
    if (r != ProtocolError::NO_ERROR) {
        return r;
    }
    CoapMessageEncoder enc((char*)coapMsg.buf(), coapMsg.capacity());
    enc.type(channel->is_unreliable() ? CoapType::CON : CoapType::NON);
    enc.code(msg->isRequest ? CoapCode::POST : CoapCode::CONTENT);
    enc.id(0); // Will be set by the message channel
    enc.token((const char*)&msg->token, sizeof(msg->token));
    enc.option(CoapOption::URI_PATH, "d");
    const uint8_t uriQuery = msg->type;
    enc.option(CoapOption::URI_QUERY, (const char*)&uriQuery, sizeof(uriQuery));
    auto payloadSize = msg->bufSize - msg->bufOffs;
    if (payloadSize > enc.maxPayloadSize() || msg->blockIndex > 0) {
        if (payloadSize > MAX_BLOCK_SIZE) {
            payloadSize = MAX_BLOCK_SIZE;
        }
        // RFC 7959, 2.2. Structure of a Block Option
        static_assert(MAX_BLOCK_SIZE == 1024 || MAX_BLOCK_SIZE == 512, "Unsupported MAX_BLOCK_SIZE");
        const unsigned szx = (MAX_BLOCK_SIZE == 1024) ? 6 /* 1024 bytes */ : 5 /* 512 bytes */;
        unsigned block1 = (msg->blockIndex << 4) | szx;
        if (msg->bufOffs + payloadSize < msg->bufSize) {
            block1 |= 0x08; // More blocks are following
        }
        enc.option(CoapOption::BLOCK1, block1);
    }
    enc.payload(msg->buf + msg->bufOffs, payloadSize);
    int n = enc.encode();
    if (n < 0 || n > (int)coapMsg.capacity()) {
        return ProtocolError::INTERNAL;
    }
    coapMsg.set_length(n);
    r = channel->send(coapMsg);
    if (r != ProtocolError::NO_ERROR) {
        return r;
    }
    msg->bufOffs += payloadSize;
    ++msg->blockIndex;
    return ProtocolError::NO_ERROR;
}

} // namespace protocol

} // namespace particle
