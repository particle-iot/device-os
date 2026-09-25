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

#include "coap_message.h"
#include "v2/coap_channel.h"

#include <string>
#include <vector>

namespace particle::test {

/**
 * Cloud-side test helper driving the real CoAP implementation (the v2 CoAP channel and the
 * coap_*() API functions).
 *
 * Messages sent by the device are recorded and can be inspected by the test. The test drives the
 * code under test by injecting connection events, server requests, responses and transport errors
 * (delivered as CoAP resets).
 *
 * Limitations: all messages are assumed to fit in a single CoAP block.
 */
class CoapFake {
public:
    // A message sent by the device
    struct Sent {
        int reqId; // Bookkeeping ID assigned by the fake
        bool isResponse;
        std::string uri;
        int method; // Requests only
        int status; // Responses only
        std::string payload;
        protocol::test::CoapMessage msg; // Encoded CoAP message as sent by the device
        bool completed; // Requests only: whether the request has been responded to
    };

    // Notify the channel that the connection is open
    void connect();
    // Notify the channel that the connection is closed
    void disconnect(int error = 0);

    bool isConnected() const {
        return connected_;
    }

    // Requests sent by the device
    const std::vector<Sent>& requests() const {
        const_cast<CoapFake*>(this)->drain(); // Record the messages that haven't been drained yet
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

    // Clear the recorded messages
    void clearLog();

    static CoapFake& instance();

private:
    std::vector<Sent> requests_;
    std::vector<Sent> responses_;
    int lastReqId_ = 0;
    bool connected_ = false;
    protocol::v2::CoapChannel* channel_ = nullptr;

    CoapFake();

    // Records the messages sent by the device since the last drain
    void drain();
};

} // namespace particle::test