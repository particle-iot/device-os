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
#include "wifi_constants.h"

system_error_t rtl_ble_error_to_system(T_GAP_CAUSE error) {
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

system_error_t rtl_error_to_system(int error) {
    switch (error) {
        case RTW_SUCCESS:
            return SYSTEM_ERROR_NONE;
        case RTW_ERROR:
        case RTW_BAD_VERSION:
        case RTW_NONRESIDENT:
            return SYSTEM_ERROR_INTERNAL;
        case RTW_PENDING:
        case RTW_NOT_AUTHENTICATED:
        case RTW_NOT_KEYED:
        case RTW_NOTUP:
        case RTW_NOTDOWN:
        case RTW_NOTAP:
        case RTW_NOTSTA:
        case RTW_RADIOOFF:
        case RTW_NOTBANDLOCKED:
        case RTW_NOCLK:
        case RTW_NOTASSOCIATED:
        case RTW_NOTREADY:
        case RTW_WME_NOT_ENABLED:
        case RTW_WLAN_DOWN:
        case RTW_NODEVICE:
        case RTW_UNFINISHED:
        case RTW_DISABLED:
        case RTW_BUFFER_UNAVAILABLE_TEMPORARY:
        case RTW_BUFFER_UNAVAILABLE_PERMANENT:
            return SYSTEM_ERROR_INVALID_STATE;
        case RTW_INVALID_KEY:
        case RTW_BADARG:
        case RTW_BADOPTION:
        case RTW_BADKEYIDX:
        case RTW_BADRATESET:
        case RTW_BADBAND:
        case RTW_BUFTOOSHORT:
        case RTW_BUFTOOLONG:
        case RTW_BADSSIDLEN:
        case RTW_OUTOFRANGECHAN:
        case RTW_BADCHAN:
        case RTW_BADADDR:
        case RTW_BADLEN:
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        case RTW_WPS_PBC_OVERLAP:
        case RTW_CONNECTION_LOST:
        case RTW_NOT_WME_ASSOCIATION:
            return SYSTEM_ERROR_NETWORK;
        case RTW_DOES_NOT_EXIST:
        case RTW_NOTFOUND:
        case RTW_TSPEC_NOTFOUND:
            return SYSTEM_ERROR_NOT_FOUND;
        case RTW_IOCTL_FAIL:
        case RTW_SDIO_ERROR:
        case RTW_TXFAIL:
        case RTW_RXFAIL:
            return SYSTEM_ERROR_IO;
        case RTW_BUSY:
            return SYSTEM_ERROR_BUSY;
        case RTW_UNSUPPORTED:
        case RTW_ACM_NOTSUPPORTED:
            return SYSTEM_ERROR_NOT_SUPPORTED;
        case RTW_NORESOURCE:
            return SYSTEM_ERROR_LIMIT_EXCEEDED;
        case RTW_EPERM:
            return SYSTEM_ERROR_NOT_ALLOWED;
        case RTW_NOMEM:
            return SYSTEM_ERROR_NO_MEMORY;
        case RTW_RANGE:
            return SYSTEM_ERROR_OUT_OF_RANGE;
        case RTW_TIMEOUT:
            return SYSTEM_ERROR_TIMEOUT;
        case RTW_PARTIAL_RESULTS:
            return SYSTEM_ERROR_NOT_ENOUGH_DATA;
        default:
            // RTW_ASSOCIATED
            return SYSTEM_ERROR_UNKNOWN;
    }
}
