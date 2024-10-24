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

#include <memory>
#include <cstdint>

#include "message_channel.h"
#include "coap_api.h"
#include "coap.h" // For token_t

#include "system_tick_hal.h"

#include "ref_count.h"

namespace particle::protocol {

class CoapMessageEncoder;
class CoapMessageDecoder;
class Protocol;

namespace experimental {

// This class implements the new experimental protocol API that allows the system to interact with
// the server at the CoAP level. It's meant to be used through the functions defined in coap_api.h
class CoapChannel {
public:
    enum Result {
        HANDLED = 1 // Returned by the handle* methods
    };

    ~CoapChannel();

    // Methods called by the new CoAP API (coap_api.h)

    int beginRequest(coap_message** msg, const char* uri, coap_method method, int timeout, int flags);
    int endRequest(coap_message* msg, coap_response_callback respCallback, coap_ack_callback ackCallback,
            coap_error_callback errorCallback, void* callbackArg);

    int beginResponse(coap_message** msg, int code, int requestId, int flags);
    int endResponse(coap_message* msg, coap_ack_callback ackCallback, coap_error_callback errorCallback,
            void* callbackArg);

    int writePayload(coap_message* msg, const char* data, size_t& size, coap_block_callback blockCallback,
            coap_error_callback errorCallback, void* callbackArg);
    int readPayload(coap_message* msg, char* data, size_t& size, coap_block_callback blockCallback,
            coap_error_callback errorCallback, void* callbackArg);
    int peekPayload(coap_message* msg, char* data, size_t size);

    int addOption(coap_message* msg, int num, const char* data, size_t size);

    void destroyMessage(coap_message* msg);

    void cancelRequest(int requestId);

    int addRequestHandler(const char* path, coap_method method, int flags, coap_request_callback callback, void* callbackArg);
    void removeRequestHandler(const char* path, coap_method method);

    int addConnectionHandler(coap_connection_callback callback, void* callbackArg);
    void removeConnectionHandler(coap_connection_callback callback);

    // Methods called by the old protocol implementation

    void open();
    void close(int error = SYSTEM_ERROR_COAP_CONNECTION_CLOSED);

    int handleCon(const Message& msg);
    int handleAck(const Message& msg);
    int handleRst(const Message& msg);

    int run();

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

    struct CoapMessage;
    struct RequestMessage;
    struct ResponseMessage;
    struct RequestHandler;
    struct ConnectionHandler;

    CoapChannel(); // Use instance()

    Message msgBuf_; // Reference to the shared message buffer
    ConnectionHandler* connHandlers_; // List of registered connection handlers
    RequestHandler* reqHandlers_; // List of registered request handlers
    RequestMessage* sentReqs_; // List of requests awaiting a response from the server
    RequestMessage* recvReqs_; // List of requests awaiting a response from the device
    ResponseMessage* blockResps_; // List of responses for which the next message block is expected to be received
    CoapMessage* unackMsgs_; // List of messages awaiting an ACK from the server
    Protocol* protocol_; // Protocol instance
    State state_; // Channel state
    uint32_t lastReqTag_; // Last used request tag
    int lastMsgId_; // Last used internal message ID
    int curMsgId_; // Internal ID of the message stored in the shared buffer
    int sessId_; // Counter incremented every time a new session with the server is started
    int pendingCloseError_; // If non-zero, the channel needs to be closed
    bool openPending_; // If true, the channel needs to be reopened

    int handleRequest(CoapMessageDecoder& d);
    int handleResponse(CoapMessageDecoder& d);
    int handleAck(CoapMessageDecoder& d);

    int prepareMessage(const RefCountPtr<CoapMessage>& msg);
    int updateMessage(const RefCountPtr<CoapMessage>& msg);
    int sendMessage(RefCountPtr<CoapMessage> msg);
    void clearMessage(const RefCountPtr<CoapMessage>& msg);

    void encodeOption(CoapMessageEncoder& e, const RefCountPtr<CoapMessage>& msg, unsigned opt, const char* data, size_t size);
    void encodeOption(CoapMessageEncoder& e, const RefCountPtr<CoapMessage>& msg, unsigned opt, unsigned val);
    void encodeOptions(CoapMessageEncoder& e, const RefCountPtr<CoapMessage>& msg);

    int sendAck(int coapId, bool rst = false);

    int handleProtocolError(ProtocolError error);

    void releaseMessageBuffer();

    system_tick_t millis() const;
};

} // namespace experimental

} // namespace particle::protocol
