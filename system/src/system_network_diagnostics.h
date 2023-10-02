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

#include "spark_wiring_diagnostics.h"
#include "system_defs.h"

namespace particle {

class NetworkDiagnostics {
public:
    // Note: Use odd numbers to encode transitional states
    enum Status {
        TURNED_OFF = 0,
        TURNING_ON = 1,
        DISCONNECTED = 2,
        CONNECTING = 3,
        CONNECTED = 4,
        DISCONNECTING = 5,
        TURNING_OFF = 7
    };

    NetworkDiagnostics() :
            status_(DIAG_ID_NETWORK_CONNECTION_STATUS, DIAG_NAME_NETWORK_CONNECTION_STATUS, TURNED_OFF),
            disconnReason_(DIAG_ID_NETWORK_DISCONNECTION_REASON, DIAG_NAME_NETWORK_DISCONNECTION_REASON, NETWORK_DISCONNECT_REASON_NONE),
            disconnCount_(DIAG_ID_NETWORK_DISCONNECTS, DIAG_NAME_NETWORK_DISCONNECTS),
            connCount_(DIAG_ID_NETWORK_CONNECTION_ATTEMPTS, DIAG_NAME_NETWORK_CONNECTION_ATTEMPTS),
            lastError_(DIAG_ID_NETWORK_CONNECTION_ERROR_CODE, DIAG_NAME_NETWORK_CONNECTION_ERROR_CODE) {
    }

    NetworkDiagnostics& status(Status status) {
        status_ = status;
        return *this;
    }

    NetworkDiagnostics& connectionAttempt() {
        ++connCount_;
        return *this;
    }

    NetworkDiagnostics& resetConnectionAttempts() {
        connCount_ = 0;
        return *this;
    }

    NetworkDiagnostics& disconnectionReason(network_disconnect_reason reason) {
        disconnReason_ = reason;
        return *this;
    }

    NetworkDiagnostics& disconnectedUnexpectedly() {
        ++disconnCount_;
        return *this;
    }

    NetworkDiagnostics& lastError(int error) {
        lastError_ = error;
        return *this;
    }

    static NetworkDiagnostics* instance();

private:
    // Some of the diagnostic data sources use the synchronization since they can be updated from
    // the networking service thread
    AtomicEnumDiagnosticData<Status> status_;
    AtomicEnumDiagnosticData<network_disconnect_reason> disconnReason_;
    AtomicUnsignedIntegerDiagnosticData disconnCount_;
    SimpleUnsignedIntegerDiagnosticData connCount_;
    SimpleIntegerDiagnosticData lastError_;
};

} // namespace particle
