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

#include "protocol_util.h"

#include "system_error.h"

// This won't work on platforms where the system part containing the comms library is not linked with Wiring
#include "spark_wiring_json.h"

#include <cstdio>

namespace particle {

namespace protocol {

int formatDiagnosticPayload(char* buf, size_t size, int error) {
    spark::JSONBufferWriter w(buf, size);
    w.beginObject();
    w.name("code").value(error);
    const size_t codeEnd = w.dataSize();
    w.name("message").value(get_error_message(error));
    w.endObject();
    size_t payloadSize = w.dataSize();
    if (size < payloadSize) {
        if (size < codeEnd + 1 /* strlen("}") */) {
            return 0;
        }
        if (size < codeEnd + 13 /* strlen(",message:\"\"}") + 1 */) {
            // Format just the code
            buf[codeEnd] = '}';
            payloadSize = codeEnd + 1;
        } else {
            // Cut some characters off the end of the message string. This may result in a malformed
            // JSON if the string contains escaped characters, but welp
            buf[size - 2] = '"';
            buf[size - 1] = '}';
            payloadSize = size;
        }
    }
    return payloadSize;
}

} // namespace protocol

} // namespace particle
