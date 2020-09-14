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

#include "coap_defs.h"

#include "system_error.h"

namespace particle {

namespace protocol {

// Note: System errors cannot be properly mapped to the response codes defined in RFC 7252.
// Most of the errors will result in 5.00 (Internal Server Error), which is used as a generic
// code and doesn't necessarily indicate an internal Device OS error
CoapCode coapCodeForSystemError(int error) {
    switch (error) {
    case SYSTEM_ERROR_NONE:
        return CoapCode::CREATED; // RFC 7252 doesn't define a generic OK code
    case SYSTEM_ERROR_INVALID_ARGUMENT:
    case SYSTEM_ERROR_BAD_DATA:
    case SYSTEM_ERROR_OUT_OF_RANGE:
    case SYSTEM_ERROR_TOO_LARGE:
    case SYSTEM_ERROR_NOT_ENOUGH_DATA:
    case SYSTEM_ERROR_COAP:
        return CoapCode::BAD_REQUEST;
    case SYSTEM_ERROR_BUSY:
    case SYSTEM_ERROR_CANCELLED:
    case SYSTEM_ERROR_ABORTED:
    case SYSTEM_ERROR_INVALID_STATE:
        return CoapCode::SERVICE_UNAVAILABLE;
    case SYSTEM_ERROR_NOT_FOUND:
        return CoapCode::NOT_FOUND;
    case SYSTEM_ERROR_NOT_ALLOWED:
        return CoapCode::FORBIDDEN;
    case SYSTEM_ERROR_TIMEOUT:
        return CoapCode::GATEWAY_TIMEOUT;
    case SYSTEM_ERROR_NOT_SUPPORTED:
    case SYSTEM_ERROR_DEPRECATED:
        return CoapCode::NOT_IMPLEMENTED;
    case SYSTEM_ERROR_NO_MEMORY:
    case SYSTEM_ERROR_INTERNAL:
    case SYSTEM_ERROR_UNKNOWN:
    default:
        return CoapCode::INTERNAL_SERVER_ERROR;
    }
}

} // namespace protocol

} // namespace particle
