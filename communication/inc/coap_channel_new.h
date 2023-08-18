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

#include "message_channel.h"
#include "coap_api.h"

#include "system_error.h"

namespace particle::protocol {

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

    int beginRequest(coap_message** msg, const char* uri, coap_method method);
    int endRequest(coap_message* msg, coap_response_callback respCallback, coap_ack_callback ackCallback,
            coap_error_callback errorCallback, void* callbackArg);

    int beginResponse(coap_message** msg, int code, int requestId);
    int endResponse(coap_message* msg, coap_ack_callback ackCallback, coap_error_callback errorCallback,
            void* callbackArg);

    int writePayload(coap_message* msg, const char* data, size_t& size, coap_block_callback blockCallback,
            coap_error_callback errorCallback, void* callbackArg);
    int readPayload(coap_message* msg, char* data, size_t& size, coap_block_callback blockCallback,
            coap_error_callback errorCallback, void* callbackArg);
    int peekPayload(coap_message* msg, char* data, size_t size);

    void destroyMessage(coap_message* msg);

    int addRequestHandler(const char* uri, coap_method method, coap_request_callback callback, void* callbackArg);
    void removeRequestHandler(const char* uri, coap_method method);

    int addConnectionHandler(coap_connection_callback callback, void* callbackArg);
    void removeConnectionHandler(coap_connection_callback callback);

    // Methods called by the old protocol implementation

    void open();
    void close(int error = SYSTEM_ERROR_COAP_CONNECTION_CLOSED);

    int handleCon(const Message& msg);
    int handleAck(const Message& msg);
    int handleRst(const Message& msg);

    static CoapChannel* instance();

private:
    struct CoapMessage;
    struct RequestHandler;
    struct ConnectionHandler;

    CoapChannel(); // Use instance()

    Message msgBuf_; // Reference to the shared message buffer
    ConnectionHandler* connHandlers_; // List of connection handlers
    RequestHandler* reqHandlers_; // List of request handlers
    CoapMessage* sentReqs_; // List of sent requests for which a response is expected
    CoapMessage* recvReqs_; // List of received requests for which a response is expected
    CoapMessage* unackMsgs_; // List of messages for which an ACK is expected
    Protocol* protocol_; // Protocol instance
    int lastMsgId_; // Last used internal message ID
    int curMsgId_; // Internal ID of the message stored in the shared buffer
    int sessId_; // Counter incremented every time a new session is started
    bool open_; // Whether the channel is open

    int handleRequest(CoapMessageDecoder& d, const Message& msg);
    int handleResponse(CoapMessageDecoder& d, const Message& msg);

    int sendEmptyAck(int coapId);

    void cancelMessages(CoapMessage* msgList, int error, bool destroy);

    int initProtocolMessage(CoapMessage* msg);
    int handleProtocolError(ProtocolError error);
};

} // namespace experimental

} // namespace particle::protocol
