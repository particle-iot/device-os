/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include "coap_message.h"

#include "coap_message_encoder.h"
#include "coap_message_decoder.h"
#include "endian_util.h"

namespace particle {

namespace protocol {

namespace test {

std::string CoapMessageOption::encodeUInt(unsigned value) {
    if (value == 0) {
        return std::string();
    } else if (value <= 0xff) {
        const uint8_t v = value;
        return std::string((const char*)&v, 1);
    } else if (value <= 0xffff) {
        const uint16_t v = nativeToBigEndian<uint16_t>(value);
        return std::string((const char*)&v, 2);
    } else if (value <= 0x00ffffff) {
        const uint32_t v = nativeToBigEndian<uint32_t>(value);
        return std::string((const char*)&v + 1, 3);
    } else {
        const uint32_t v = nativeToBigEndian<uint32_t>(value);
        return std::string((const char*)&v, 4);
    }
}

unsigned CoapMessageOption::decodeUInt(const std::string& data) {
    if (data.size() > 4) {
        throw std::runtime_error("Unable to decode CoAP option");
    }
    uint32_t v = 0;
    memcpy((char*)&v + 4 - data.size(), data.data(), data.size());
    return bigEndianToNative(v);
}

std::string CoapMessage::encode() const {
    std::string buf;
    for (;;) {
        CoapMessageEncoder e(&buf.front(), buf.size());
        e.type(type());
        e.code(code());
        e.id(id());
        if (hasToken()) {
            const auto& s = token();
            e.token(s.data(), s.size());
        }
        for (auto opt: options()) {
            const auto& s = opt.toString();
            e.option(opt.number(), s.data(), s.size());
        }
        if (hasPayload()) {
            const auto& s = payload();
            e.payload(s.data(), s.size());
        }
        const auto r = e.encode();
        if (r <= 0) {
            throw std::runtime_error("Unable to encode CoAP message");
        }
        if (!buf.empty()) {
            break;
        }
        buf.resize(r);
    }
    return buf;
}

CoapMessage CoapMessage::decode(const char* data, size_t size) {
    CoapMessage msg;
    CoapMessageDecoder d;
    if (d.decode(data, size) < 0) {
        throw std::runtime_error("Unable to decode CoAP message");
    }
    msg.type(d.type());
    msg.code(d.code());
    msg.id(d.id());
    if (d.hasToken()) {
        msg.token(d.token(), d.tokenSize());
    }
    auto it = d.options();
    while (it.next()) {
        msg.option(it.option(), it.data(), it.size());
    }
    if (d.hasPayload()) {
        msg.payload(d.payload(), d.payloadSize());
    }
    return msg;
}

} // namespace test

} // namespace protocol

} // namespace particle
