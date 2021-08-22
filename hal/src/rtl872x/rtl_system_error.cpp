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

#include "rtl_system_error.h"

system_error_t rtl_system_error(T_GAP_CAUSE error) {
    switch (error) {
        case GAP_CAUSE_SUCCESS:
            return SYSTEM_ERROR_NONE;
        case GAP_CAUSE_NON_CONN:
        case GAP_CAUSE_INVALID_STATE:
        case GAP_CAUSE_NO_BOND:
            return SYSTEM_ERROR_INVALID_STATE;
        case GAP_CAUSE_ERROR_CREDITS:
        case GAP_CAUSE_SEND_REQ_FAILED:
            return SYSTEM_ERROR_INTERNAL;
        case GAP_CAUSE_NO_RESOURCE:
            return SYSTEM_ERROR_NO_MEMORY;
        case GAP_CAUSE_NOT_FIND_IRK:
        case GAP_CAUSE_NOT_FIND:
            return SYSTEM_ERROR_NOT_FOUND;
        case GAP_CAUSE_INVALID_PARAM:
        case GAP_CAUSE_INVALID_PDU_SIZE:
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        case GAP_CAUSE_ALREADY_IN_REQ:
            return SYSTEM_ERROR_BUSY;
        case GAP_CAUSE_CONN_LIMIT:
            return SYSTEM_ERROR_LIMIT_EXCEEDED;
        default:
            return SYSTEM_ERROR_UNKNOWN;
    }
}
