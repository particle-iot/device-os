/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include <cstddef>

#include "coap_api.h"

#include <string>
#include <vector>
#include <map>
#include <set>

namespace particle::test {

/**
 * A fake implementation of the CoAP API (coap_api.h) that plays the role of the cloud.
 *
 * Messages sent by the device are recorded and can be inspected by the test. The test drives the
 * code under test by injecting connection events, server requests, responses and errors.
 *
 * Limitations: all messages are assumed to fit in a single CoAP block.
 */
class CoapFake {
public:
    // A message sent by the device
    struct Sent {
        int reqId;
        bool isResponse;
        std::string uri;
        int method; // Requests only
        int status; // Responses only
        std::string payload;
        coap_response_callback respCb;
        coap_error_callback errorCb;
        void* arg;
    };

    // Notify the connection handlers that the connection is open
    void connect();
    // Notify the connection handlers that the connection is closed
    void disconnect(int error = 0);

    bool isConnected() const {
        return connected_;
    }

    // Requests sent by the device
    const std::vector<Sent>& requests() const {
        return requests_;
    }
    // Last request sent by the device, or null if no requests were sent
    const Sent* lastRequest() const;

    // Complete a request sent by the device with a response
    void respond(int reqId, int status, const std::string& payload = std::string());
    // Fail a request sent by the device
    void failRequest(int reqId, int error);

    // Send a request to the device. Returns the request ID
    int sendRequest(const char* uri, int method, const std::string& payload);
    // Response sent by the device to a request sent via sendRequest(), or null
    const Sent* responseFor(int reqId) const;
    // Fail a response sent by the device
    void failResponse(int reqId, int error);

    // IDs of the requests cancelled via coap_cancel_request()
    const std::vector<int>& cancelled() const {
        return cancelled_;
    }

    // Clear the recorded messages. Registered handlers are kept
    void clearLog();

    static CoapFake& instance();

    // Methods called by the fake coap_api functions
    struct ConnHandler {
        coap_connection_callback cb;
        void* arg;
    };

    struct ReqHandler {
        std::string uri;
        int method;
        coap_request_callback cb;
        void* arg;
    };

    std::vector<ConnHandler> connHandlers;
    std::vector<ReqHandler> reqHandlers;

    int nextReqId();
    void requestSent(Sent sent);
    void responseSent(Sent sent);
    bool takeIncomingRequest(int reqId);
    void requestCancelled(int reqId);

private:
    std::vector<Sent> requests_;
    std::vector<Sent> responses_;
    std::map<int, Sent> pending_; // Requests awaiting a response
    std::set<int> incoming_; // Requests from the "cloud" awaiting a response
    std::vector<int> cancelled_;
    int lastReqId_ = 0;
    bool connected_ = false;

    CoapFake() = default;
};

} // namespace particle::test
