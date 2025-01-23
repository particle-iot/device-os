/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#include <memory>
#include <cstring>
#include <cstdint>

#include "message_channel.h"
#include "coap_tag.h"
#include "coap_defs.h"
#include "coap_api.h"

#include "system_tick_hal.h"

#include "ref_count.h"

namespace particle::protocol {

class CoapMessageDecoder;
class Protocol;

namespace v2 {

class CoapTag;
typedef CoapTag CoapToken;

class CoapPayload;
class CoapOptionEntry;

/**
 * Base abstract class for a CoAP message.
 */
class CoapMessage: public RefCount {
public:
    virtual ~CoapMessage() = default;

    // TODO: Expose the relevant methods of CoapChannel (setPayload(), addOption(), etc.) as methods
    // of this class instead
};

/**
 * CoAP protocol.
 *
 * This class implements the new protocol API that allows the system to interact with the server at
 * the CoAP level. It's meant to be used through the functions defined in coap_api.h
 */
class CoapChannel {
public:
    using MessageBuffer = particle::protocol::Message;

    enum Result {
        HANDLED = 1 // Returned by the handle* methods
    };

    ~CoapChannel();

    // Methods called by the CoAP API functions (coap_api.h)

    int beginRequest(RefCountPtr<CoapMessage>& msg, const char* path, coap_method method, int timeout, int flags);
    int endRequest(RefCountPtr<CoapMessage> msg, coap_response_callback respCallback, coap_ack_callback ackCallback,
            coap_error_callback errorCallback, void* callbackArg);

    int beginResponse(RefCountPtr<CoapMessage>& msg, int code, int reqId, int flags);
    int endResponse(RefCountPtr<CoapMessage> msg, coap_ack_callback ackCallback, coap_error_callback errorCallback,
            void* callbackArg);

    int writeBlock(const RefCountPtr<CoapMessage>& msg, const char* data, size_t& size, coap_block_callback blockCallback,
            coap_error_callback errorCallback, void* callbackArg);
    int readBlock(const RefCountPtr<CoapMessage>& msg, char* data, size_t& size, coap_block_callback blockCallback,
            coap_error_callback errorCallback, void* callbackArg);
    int peekBlock(const RefCountPtr<CoapMessage>& msg, char* data, size_t size);

    int setPayload(const RefCountPtr<CoapMessage>& msg, RefCountPtr<CoapPayload> payload);
    int getPayload(const RefCountPtr<CoapMessage>& msg, RefCountPtr<CoapPayload>& payload);

    int addOption(const RefCountPtr<CoapMessage>& msg, unsigned num, const char* data, size_t size);
    int addOption(const RefCountPtr<CoapMessage>& msg, unsigned num, unsigned val);
    int getOption(const RefCountPtr<CoapMessage>& msg, unsigned num, const CoapOptionEntry*& opt);
    int getFirstOption(const RefCountPtr<CoapMessage>& msg, const CoapOptionEntry*& opt);

    int cancelRequest(int reqId);

    void disposeMessage(const RefCountPtr<CoapMessage>& msg);

    int addRequestHandler(const char* path, coap_method method, int flags, coap_request_callback callback, void* callbackArg);
    void removeRequestHandler(const char* path, coap_method method);

    int addConnectionHandler(coap_connection_callback callback, void* callbackArg);
    void removeConnectionHandler(coap_connection_callback callback);

    // Methods called by the system

    void open();
    void close(int error = SYSTEM_ERROR_COAP_CONNECTION_CLOSED);

    int run();

    // Methods called by the old protocol implementation

    int handleCon(const MessageBuffer& buf);
    int handleAck(const MessageBuffer& buf);
    int handleRst(const MessageBuffer& buf);

    static CoapChannel* instance();

private:
    // Channel state
    enum class State {
        CLOSED,
        OPENING,
        OPEN,
        CLOSING
    };

    enum class MessageType {
        REQUEST, // Regular or blockwise request carrying request data
        BLOCK_REQUEST, // Blockwise request retrieving a block of response data
        RESPONSE // Regular or blockwise response
    };

    enum class MessageState {
        NEW, // Message created
        READ, // Reading payload data
        WRITE, // Writing payload data
        WAIT_ACK, // Waiting for an ACK
        WAIT_RESPONSE, // Waiting for a response
        WAIT_BLOCK, // Waiting for the next message block
        DONE // Message exchange completed
    };

    struct Message;
    struct RequestMessage;
    struct ResponseMessage;
    struct RequestHandler;
    struct ConnectionHandler;
    struct EncodingContext;

    CoapChannel(); // Use instance()

    MessageBuffer msgBuf_; // Reference to the shared message buffer
    ConnectionHandler* connHandlers_; // List of registered connection handlers
    RequestHandler* reqHandlers_; // List of registered request handlers
    RequestMessage* sentReqs_; // List of requests awaiting a response from the server
    RequestMessage* recvReqs_; // List of requests awaiting a response from the device
    RequestMessage* recvBlockReqs_; // List of incoming blockwise requests in progress
    ResponseMessage* blockResps_; // List of responses for which the next message block is expected to be received
    Message* unackMsgs_; // List of messages awaiting an ACK from the server
    Protocol* protocol_; // Protocol instance
    CoapTag lastReqTag_; // Last used request tag
    State state_; // Channel state
    int lastMsgId_; // Last used internal message ID
    int curMsgId_; // Internal ID of the message stored in the shared buffer
    int sessId_; // Counter incremented every time a new session with the server is started
    int pendingCloseError_; // If non-zero, the channel needs to be closed
    bool openPending_; // If true, the channel needs to be reopened

    int handleRequest(CoapMessageDecoder& d);
    int handleRequestImpl(CoapMessageDecoder& d, CoapCode& errStatus);
    int handleResponse(CoapMessageDecoder& d);
    int handleAck(CoapMessageDecoder& d);

    int prepareMessage(const RefCountPtr<Message>& msg, bool retransmit = false);
    int updateMessage(const RefCountPtr<Message>& msg);
    int sendMessage(RefCountPtr<Message> msg, bool retransmit = false, bool passthrough = false);
    int sendPayloadBlock(const RefCountPtr<Message>& msg, bool retransmit = false);
    void clearMessage(const RefCountPtr<Message>& msg);

    int sendResponseAck(CoapCode code);
    int sendEmptyAck();

    void encodeOption(EncodingContext& ctx, CoapOption num, const char* data, size_t size);
    void encodeOption(EncodingContext& ctx, CoapOption num, unsigned val);
    void encodeOptions(EncodingContext& ctx, unsigned lastNumToEncode = MAX_COAP_OPTION_NUMBER);

    int handleProtocolError(ProtocolError error);

    void releaseMessageBuffer();

    system_tick_t millis() const;
};

} // namespace v2

} // namespace particle::protocol
