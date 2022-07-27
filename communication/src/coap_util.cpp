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

#include <algorithm>
#include <cstdio>

#include "coap_util.h"
#include "coap_message_decoder.h"
#include "str_util.h"

namespace particle::protocol {

void logCoapMessage(LogLevel level, const char* category, const char* data, size_t size, bool logPayload) {
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
}

} // namespace particle::protocol
