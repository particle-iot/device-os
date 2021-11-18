/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

class Appender;

namespace protocol {

class CoapMessageDecoder;
class Protocol;
class Message;

class Description {
public:
    explicit Description(Protocol* proto);
    ~Description();

    ProtocolError sendRequest(int flags);
    ProtocolError processRequest(const Message& msg);
    ProtocolError processAck(const Message& msg, int* flags);
    ProtocolError processTimeouts();

    ProtocolError serialize(Appender* appender, int flags);

    void reset();

private:
    struct Request {
        Vector<char> data; // Describe data
        message_id_t msgId; // Message ID of the last sent request
        unsigned blockIndex; // Index of the next block to send
        int flags; // Describe flags
    };

    struct Response {
        Vector<char> data; // Describe data
        system_tick_t accessTime; // Last resource access time
        unsigned blockCount; // Number of blocks comprising the resource data
        unsigned reqCount; // Number of concurrent requests accessing this resource
        unsigned etag; // ETag option
        int flags; // Describe flags
    };

    struct Ack {
        message_id_t msgId; // Message ID
        int flags; // Describe flags
    };

    Vector<Request> reqs_; // Describe requests
    Vector<Response> resps_; // Describe responses
    Vector<Ack> acks_; // Expected acknowledgements
    Protocol* proto_; // Protocol instance
    unsigned lastEtag_; // Last used ETag

    ProtocolError sendNextRequestBlock(Request* req, bool* hasMore);
    ProtocolError sendResponseBlock(const Response& resp, token_t token, unsigned blockIndex, message_id_t* msgId, bool* hasMore);
    ProtocolError sendErrorResponse(const CoapMessageDecoder& reqDec, CoapCode code);
    ProtocolError sendEmptyAck(const CoapMessageDecoder& reqDec);
    ProtocolError getDescribeData(Vector<char>* data, int flags);
    system_tick_t millis() const;
};

} // namespace protocol

} // namespace particle
