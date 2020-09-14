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

#pragma once

#include "coap.h"

#include "underlying_type.h"

#include <cstdint>

namespace particle {

namespace protocol {

const size_t MIN_COAP_MESSAGE_SIZE = 4;

const size_t MAX_COAP_TOKEN_SIZE = 8;

constexpr unsigned coapCode(unsigned cls, unsigned detail) {
    return ((cls & 0x07) << 5) | (detail & 0x1f);
}

enum class CoapType {
    CON = 0,
    NON = 1,
    ACK = 2,
    RST = 3
};

PARTICLE_DEFINE_ENUM_COMPARISON_OPERATORS(CoapType)

// RFC 7252, 12.1. CoAP Code Registries
enum class CoapCode {
    EMPTY = coapCode(0, 0),
    // Method codes
    GET = coapCode(0, 1),
    POST = coapCode(0, 2),
    PUT = coapCode(0, 3),
    DELETE = coapCode(0, 4),
    // Response codes
    CREATED = coapCode(2, 1),
    DELETED = coapCode(2, 2),
    VALID = coapCode(2, 3),
    CHANGED = coapCode(2, 4),
    CONTENT = coapCode(2, 5),
    BAD_REQUEST = coapCode(4, 0),
    UNAUTHORIZED = coapCode(4, 1),
    BAD_OPTION = coapCode(4, 2),
    FORBIDDEN = coapCode(4, 3),
    NOT_FOUND = coapCode(4, 4),
    METHOD_NOT_ALLOWED = coapCode(4, 5),
    NOT_ACCEPTABLE = coapCode(4, 6),
    PRECONDITION_FAILED = coapCode(4, 12),
    REQUEST_ENTITY_TOO_LARGE = coapCode(4, 13),
    UNSUPPORTED_CONTENT_FORMAT = coapCode(4, 15),
    INTERNAL_SERVER_ERROR = coapCode(5, 0),
    NOT_IMPLEMENTED = coapCode(5, 1),
    BAD_GATEWAY = coapCode(5, 2),
    SERVICE_UNAVAILABLE = coapCode(5, 3),
    GATEWAY_TIMEOUT = coapCode(5, 4),
    PROXYING_NOT_SUPPORTED = coapCode(5, 5)
};

PARTICLE_DEFINE_ENUM_COMPARISON_OPERATORS(CoapCode)

// RFC 7252, 5.10. Option Definitions
enum class CoapOption {
    IF_MATCH = 1,
    URI_HOST = 3,
    ETAG = 4,
    IF_NONE_MATCH = 5,
    URI_PORT = 7,
    LOCATION_PATH = 8,
    URI_PATH = 11,
    CONTENT_FORMAT = 12,
    MAX_AGE = 14,
    URI_QUERY = 15,
    ACCEPT = 17,
    LOCATION_QUERY = 20,
    PROXY_URI = 35,
    PROXY_SCHEME = 39,
    SIZE1 = 60
};

PARTICLE_DEFINE_ENUM_COMPARISON_OPERATORS(CoapOption)

typedef uint16_t CoapMessageId;

unsigned coapCodeClass(unsigned code);
unsigned coapCodeDetail(unsigned code);
bool isCoapRequestCode(unsigned code);
bool isCoapResponseCode(unsigned code);
bool isCoapSuccessCode(unsigned code);
bool isCoapValidCode(unsigned code);

bool isCoapRequestType(CoapType type);
bool isCoapResponseType(CoapType type);
bool isCoapValidType(unsigned type);

bool isCoapRequest(CoapType type, unsigned code);
bool isCoapResponse(CoapType type, unsigned code);
bool isCoapEmptyAck(CoapType type, unsigned code);

CoapCode coapCodeForSystemError(int error);

inline unsigned coapCodeClass(unsigned code) {
    return (code >> 5) & 0x07;
}

inline unsigned coapCodeDetail(unsigned code) {
    return code & 0x1f;
}

inline bool isCoapRequestCode(unsigned code) {
    return code == CoapCode::GET || code == CoapCode::POST || code == CoapCode::PUT || code == CoapCode::DELETE;
}

inline bool isCoapResponseCode(unsigned code) {
    const auto cls = coapCodeClass(code);
    return cls == 2 || cls == 4 || cls == 5;
}

inline bool isCoapSuccessCode(unsigned code) {
    return coapCodeClass(code) == 2;
}

inline bool isCoapValidCode(unsigned code) {
    const auto cls = coapCodeClass(code);
    return cls == 0 || cls == 2 || cls == 4 || cls == 5;
}

inline bool isCoapRequestType(CoapType type) {
    return type == CoapType::CON || type == CoapType::NON;
}

inline bool isCoapResponseType(CoapType type) {
    return type == CoapType::CON || type == CoapType::NON || type == CoapType::ACK;
}

inline bool isCoapValidType(unsigned type) {
    return type == CoapType::CON || type == CoapType::NON || type == CoapType::ACK || type == CoapType::RST;
}

inline bool isCoapRequest(CoapType type, unsigned code) {
    return isCoapRequestType(type) && isCoapRequestCode(code);
}

inline bool isCoapResponse(CoapType type, unsigned code) {
    return isCoapResponseType(type) && isCoapResponseCode(code);
}

inline bool isCoapEmptyAck(CoapType type, unsigned code) {
    return type == CoapType::ACK && code == CoapCode::EMPTY;
}

} // namespace protocol

} // namespace particle
