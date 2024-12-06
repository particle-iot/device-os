/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#if !defined(DEBUG_BUILD) && !defined(UNIT_TEST)
#define NDEBUG // TODO: Define NDEBUG in release builds
#endif

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cassert>

#include "coap_util.h"

#include "message_channel.h"
#include "coap_message_encoder.h"
#include "coap_message_decoder.h"
#include "str_util.h"
#include "endian_util.h"
#include "logging.h"
#include "scope_guard.h"
#include "check.h"

namespace particle::protocol {

namespace {

int parseMessageHeader(const Message& msg, CoapMessageId& id, char token[MAX_COAP_TOKEN_SIZE], size_t& tokenSize) {
    // Not using CoapMessageDecoder as it parses the entire message
    auto msgLen = msg.length();
    if (msgLen < 4) {
        return SYSTEM_ERROR_NOT_ENOUGH_DATA;
    }
    auto msgData = msg.buf();
    uint32_t h = 0;
    std::memcpy(&h, msgData, 4);
    h = bigEndianToNative(h);
    size_t tkl = (h >> 24) & 0x0f;
    if (tkl > MAX_COAP_TOKEN_SIZE) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    if (msgLen < tkl + 4) {
        return SYSTEM_ERROR_NOT_ENOUGH_DATA;
    }
    std::memcpy(token, msgData + 4, tkl);
    tokenSize = tkl;
    id = h & 0xffff;
    return 0;
}

int sendResponse(MessageChannel& channel, Message& msg, CoapType type, CoapCode code = CoapCode::EMPTY,
        const char* payload = nullptr, size_t payloadSize = 0) {
    // Get the ID and token of the original message
    char token[MAX_COAP_TOKEN_SIZE];
    size_t tokenSize;
    CoapMessageId id;
    CHECK(parseMessageHeader(msg, id, token, tokenSize));
    // Allocate a buffer for a response message
    auto msgLen = msg.length();
    auto msgCapacity = msg.capacity();
    Message resp;
    auto err = channel.response(msg, resp, msgCapacity - msgLen);
    if (err != ProtocolError::NO_ERROR) {
        return toSystemError(err);
    }
    SCOPE_GUARD({
        // BufferMessageChannel::response() alters the capacity of the original message. Restore it
        // so that the calling code could still use the extra space available in the message
        msg.set_buffer(msg.buf(), msgCapacity);
        msg.set_length(msgLen);
    });
    // Serialize a response
    CoapMessageEncoder e((char*)resp.buf(), resp.capacity());
    e.type(type);
    e.code(code);
    e.id(0); // Serialized by the message channel
    if (code != CoapCode::EMPTY) {
        e.token(token, tokenSize);
    }
    e.payload(payload, payloadSize);
    size_t n = CHECK(e.encode());
    if (n > resp.capacity()) {
        LOG(ERROR, "No enough space in CoAP message buffer");
        return SYSTEM_ERROR_TOO_LARGE;
    }
    resp.set_length(n);
    if (type == CoapType::ACK || type == CoapType::RST) {
        resp.set_id(id);
    }
    // Send the response
    err = channel.send(resp);
    if (err != ProtocolError::NO_ERROR) {
        return toSystemError(err);
    }
    return resp.get_id();
}

} // namespace

int sendResponseAck(MessageChannel& channel, Message& msg, CoapCode code, const char* payload, size_t payloadSize) {
    CHECK(sendResponse(channel, msg, CoapType::ACK, code, payload, payloadSize));
    return 0;
}

int sendEmptyAck(MessageChannel& channel, Message& msg) {
    CHECK(sendResponse(channel, msg, CoapType::ACK, CoapCode::EMPTY));
    return 0;
}

int sendRst(MessageChannel& channel, Message& msg) {
    CHECK(sendResponse(channel, msg, CoapType::RST));
    return 0;
}

size_t appendUriPath(char* buf, size_t bufSize, size_t pathLen, const CoapOptionIterator& it) {
    assert(it.option() == CoapOption::URI_PATH);
    auto end = buf + bufSize;
    buf += pathLen;
    if (buf < end) {
        if (pathLen > 0) {
            *buf++ = '/';
        }
        size_t n = std::min<size_t>(it.size(), end - buf);
        std::memcpy(buf, it.data(), n);
        buf += n;
        if (buf == end) {
            --buf;
        }
        *buf = '\0';
    }
    return it.size() + ((pathLen > 0) ? 1 : 0);
}

void logCoapMessage(LogLevel level, const char* category, const char* data, size_t size, bool logPayload) {
#ifndef LOG_DISABLE
    CoapMessageDecoder d;
    const int r = d.decode(data, size);
    if (r < 0) {
        return;
    }
    const char* type = "";
    switch (d.type()) {
    case CoapType::CON:
        type = "CON";
        break;
    case CoapType::NON:
        type = "NON";
        break;
    case CoapType::ACK:
        type = "ACK";
        break;
    case CoapType::RST:
        type = "RST";
        break;
    }
    char code[16] = {};
    switch (d.code()) {
    case (unsigned)CoapCode::GET:
        snprintf(code, sizeof(code), "GET");
        break;
    case (unsigned)CoapCode::POST:
        snprintf(code, sizeof(code), "POST");
        break;
    case (unsigned)CoapCode::PUT:
        snprintf(code, sizeof(code), "PUT");
        break;
    case (unsigned)CoapCode::DELETE:
        snprintf(code, sizeof(code), "DELETE");
        break;
    default:
        const auto cls = coapCodeClass(d.code());
        const auto detail = coapCodeDetail(d.code());
        snprintf(code, sizeof(code), "%u.%02u", (unsigned)cls, (unsigned)detail);
        break;
    }
    char token[24] = {};
    if (d.hasToken()) {
        toHex(d.token(), d.tokenSize(), token, sizeof(token));
    }
    char uri[64] = {};
    size_t pos = 0;
    bool hasQuery = false;
    auto it = d.options();
    while (it.next() && pos + 1 < sizeof(uri)) {
        if (it.option() == CoapOption::URI_PATH || it.option() == CoapOption::URI_QUERY) {
            if (it.option() == CoapOption::URI_PATH) {
                uri[pos++] = '/';
            } else if (!hasQuery) {
                uri[pos++] = '?';
                hasQuery = true;
            } else {
                uri[pos++] = '&';
            }
            pos += toPrintable(it.data(), it.size(), uri + pos, sizeof(uri) - pos - 1);
        }
    }
    _LOG_ATTR_INIT(attr);
    log_message(level, category, &attr, nullptr /* reserved */, "%s %s %s size=%u token=%s id=%u", type, code, uri,
            (unsigned)size, token, (unsigned)d.id());
    if (logPayload && d.hasPayload()) {
        log_printf(level, category, nullptr, "Payload (%u bytes): ", (unsigned)d.payloadSize());
        log_dump(level, category, d.payload(), d.payloadSize(), 0 /* flags */, nullptr /* reserved */);
        log_write(level, category, "\r\n", 2 /* size */, nullptr /* reserved */);
    }
#endif // !defined(LOG_DISABLE)
}

} // namespace particle::protocol
