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

#include "hal_platform.h"

#include <memory>
#include <optional>
#include <cstdint>

namespace particle {

class Appender;

namespace protocol {

class CoapMessageEncoder;
class CoapMessageDecoder;
class Protocol;
class Message;

const system_tick_t COAP_BLOCKWISE_RESPONSE_TIMEOUT = 90000;

class Description {
public:
    explicit Description(Protocol* proto);
    ~Description();

    ProtocolError sendRequest(int descFlags);
    ProtocolError receiveRequest(const Message& msg);
    ProtocolError receiveAckOrRst(const Message& msg, int* descFlags);
    ProtocolError processTimeouts();

    ProtocolError serialize(Appender* appender, int descFlags);

    void reset();

private:
    struct Request {
        Vector<char> data; // Remaining Describe data
        message_id_t msgId; // Message ID of the last sent block request
        unsigned nextBlockIndex; // Index of the next block to send
        int flags; // Describe flags
    };

    struct Response {
        Vector<char> data; // Describe data
        system_tick_t lastAccessTime; // Last time this Describe data was requested
        unsigned blockCount; // Number of blocks required to transfer the Describe data
        unsigned reqCount; // Number of concurrent blockwise transfers requesting the Describe data
        unsigned etag; // ETag option
        int flags; // Describe flags
    };

    struct Ack {
        message_id_t msgId; // Message ID
        int flags; // Describe flags
    };

    std::optional<Request> activeReq_; // Blockwise Describe request that is being sent to the server
    Vector<Response> activeResps_; // Blockwise Describe responses that are being sent to the server
    Vector<int> reqQueue_; // Queued Describe requests (flags)
    Vector<Ack> acks_; // Pending acknowledgements for device-originated messages
    Protocol* proto_; // Protocol instance
    size_t blockSize_; // Block size used for blockwise transfers
    unsigned lastEtag_; // Last used ETag

    ProtocolError sendNextRequest(int flags);
    ProtocolError sendNextRequestBlock(Request* req, Message* msg, token_t token);
    ProtocolError sendResponseBlock(const Response& resp, Message* msg, token_t token, unsigned blockIndex);
    ProtocolError sendErrorResponse(const CoapMessageDecoder& reqDec, CoapCode code);
    ProtocolError sendEmptyAck(message_id_t msgId);
    ProtocolError encodeAndSend(CoapMessageEncoder* enc, Message* msg);
    ProtocolError getDescribeData(int flags, Message* msg, size_t msgOffs, Vector<char>* buf, size_t* size);
    ProtocolError getBlockSize(size_t* size);
    system_tick_t millis() const;
};

} // namespace protocol

} // namespace particle
