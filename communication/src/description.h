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

#include "protocol_defs.h"
#include "coap_defs.h"

#include "spark_wiring_vector.h"

#include <memory>
#include <cstdint>

namespace particle {

namespace protocol {

class Protocol;
class Message;

class Description {
public:
    explicit Description(Protocol* proto);
    ~Description();

    ProtocolError beginRequest(int type);
    ProtocolError beginResponse(int type, token_t token);
    ProtocolError write(const char* data, size_t size);
    ProtocolError send();
    void cancel();

    ProtocolError processAck(const Message* coapMsg, int* type = nullptr);
    ProtocolError processTimeouts();

    void reset();

private:
    struct DescribeMessage; // Describe request or response

    Vector<std::unique_ptr<DescribeMessage>> msgs_; // Describe requests and responses that are being sent
    std::unique_ptr<DescribeMessage> curMsg_; // Describe request or response that is being serialized
    Protocol* proto_; // Protocol instance

    ProtocolError sendNext(DescribeMessage* msg);
};

} // namespace protocol

} // namespace particle
