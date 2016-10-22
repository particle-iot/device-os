/*
 * Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.
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

#include "protocol_defs.h"

system_error particle::protocol::toSystemError(ProtocolError error) {
    switch (error) {
    case NO_ERROR:
        return SYSTEM_ERROR_NONE;
    case PING_TIMEOUT:
    case MESSAGE_TIMEOUT:
        return SYSTEM_ERROR_TIMEOUT;
    case IO_ERROR:
        return SYSTEM_ERROR_IO;
    case INVALID_STATE:
        return SYSTEM_ERROR_INVALID_STATE;
    case AUTHENTICATION_ERROR:
        return SYSTEM_ERROR_NOT_ALLOWED;
    case BANDWIDTH_EXCEEDED:
        return SYSTEM_ERROR_LIMIT_EXCEEDED;
    case INSUFFICIENT_STORAGE:
        return SYSTEM_ERROR_TOO_LARGE;
    default:
        return SYSTEM_ERROR_PROTOCOL; // Generic protocol error
    }
}
