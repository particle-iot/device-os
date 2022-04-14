/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include "nrf_system_error.h"

system_error_t nrf_system_error(uint32_t error) {
    switch (error) {
        case NRF_SUCCESS:
            return SYSTEM_ERROR_NONE;
        case NRF_ERROR_SVC_HANDLER_MISSING:
        case NRF_ERROR_NOT_SUPPORTED:
            return SYSTEM_ERROR_NOT_SUPPORTED;
        case NRF_ERROR_SOFTDEVICE_NOT_ENABLED:
        case NRF_ERROR_INVALID_STATE:
            return SYSTEM_ERROR_INVALID_STATE;
        case NRF_ERROR_INTERNAL:
            return SYSTEM_ERROR_INTERNAL;
        case NRF_ERROR_NO_MEM:
            return SYSTEM_ERROR_NO_MEMORY;
        case NRF_ERROR_NOT_FOUND:
            return SYSTEM_ERROR_NOT_FOUND;
        case NRF_ERROR_INVALID_PARAM:
        case NRF_ERROR_INVALID_LENGTH:
        case NRF_ERROR_INVALID_FLAGS:
        case NRF_ERROR_INVALID_DATA:
        case NRF_ERROR_DATA_SIZE:
        case NRF_ERROR_NULL:
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        case NRF_ERROR_TIMEOUT:
            return SYSTEM_ERROR_TIMEOUT;
        case NRF_ERROR_FORBIDDEN:
        case NRF_ERROR_INVALID_ADDR:
            return SYSTEM_ERROR_NOT_ALLOWED;
        case NRF_ERROR_BUSY:
            return SYSTEM_ERROR_BUSY;
#ifdef SOFTDEVICE_PRESENT
        case NRF_ERROR_CONN_COUNT:
            return SYSTEM_ERROR_LIMIT_EXCEEDED;
        case NRF_ERROR_RESOURCES:
            return SYSTEM_ERROR_ABORTED;
#endif
        default:
            return SYSTEM_ERROR_UNKNOWN;
    }
}
