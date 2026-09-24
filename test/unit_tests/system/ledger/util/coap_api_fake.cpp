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

#include "util/coap_api_fake.h"

#include "system_error.h"

#include <algorithm>
#include <stdexcept>
#include <cstring>

struct coap_message {
    std::string uri;
    std::string payload;
    size_t readPos = 0;
    int method = 0;
    int status = 0;
    int reqId = 0;
    bool isResponse = false;
};

namespace particle::test {

void CoapFake::connect() {
    connected_ = true;
    auto handlers = connHandlers; // Handlers may be modified by the callbacks
    for (auto& h: handlers) {
        h.cb(0 /* error */, COAP_CONNECTION_OPEN, h.arg);
    }
}

void CoapFake::disconnect(int error) {
    connected_ = false;
    pending_.clear();
    incoming_.clear();
    auto handlers = connHandlers;
    for (auto& h: handlers) {
        h.cb(error, COAP_CONNECTION_CLOSED, h.arg);
    }
}

const CoapFake::Sent* CoapFake::lastRequest() const {
    if (requests_.empty()) {
        return nullptr;
    }
    return &requests_.back();
}

void CoapFake::respond(int reqId, int status, const std::string& payload) {
    auto it = pending_.find(reqId);
    if (it == pending_.end()) {
        throw std::runtime_error("Unknown request ID");
    }
    auto req = it->second;
    pending_.erase(it);
    auto msg = new coap_message();
    msg->payload = payload;
    msg->status = status;
    msg->reqId = reqId;
    msg->isResponse = true;
    // The response callback takes ownership over the message
    if (req.respCb) {
        req.respCb(msg, status, reqId, req.arg);
    } else {
        delete msg;
    }
}

void CoapFake::failRequest(int reqId, int error) {
    auto it = pending_.find(reqId);
    if (it == pending_.end()) {
        throw std::runtime_error("Unknown request ID");
    }
    auto req = it->second;
    pending_.erase(it);
    if (req.errorCb) {
        req.errorCb(error, reqId, req.arg);
    }
}

int CoapFake::sendRequest(const char* uri, int method, const std::string& payload) {
    auto it = std::find_if(reqHandlers.begin(), reqHandlers.end(), [&](const ReqHandler& h) {
        return h.uri == uri && h.method == method;
    });
    if (it == reqHandlers.end()) {
        throw std::runtime_error("No request handler registered");
    }
    auto h = *it;
    int reqId = nextReqId();
    incoming_.insert(reqId);
    auto msg = new coap_message();
    msg->uri = uri;
    msg->method = method;
    msg->payload = payload;
    msg->reqId = reqId;
    // The request callback takes ownership over the message
    h.cb(msg, uri, method, reqId, h.arg);
    return reqId;
}

const CoapFake::Sent* CoapFake::responseFor(int reqId) const {
    auto it = std::find_if(responses_.begin(), responses_.end(), [reqId](const Sent& s) {
        return s.reqId == reqId;
    });
    if (it == responses_.end()) {
        return nullptr;
    }
    return &*it;
}

void CoapFake::failResponse(int reqId, int error) {
    auto resp = responseFor(reqId);
    if (!resp) {
        throw std::runtime_error("Unknown request ID");
    }
    if (resp->errorCb) {
        resp->errorCb(error, reqId, resp->arg);
    }
}

void CoapFake::clearLog() {
    requests_.clear();
    responses_.clear();
    pending_.clear();
    incoming_.clear();
    cancelled_.clear();
}

CoapFake& CoapFake::instance() {
    // Intentionally leaked so that the fake outlives static objects that use it in their destructors
    static auto fake = new CoapFake();
    return *fake;
}

int CoapFake::nextReqId() {
    return ++lastReqId_;
}

void CoapFake::requestSent(Sent sent) {
    requests_.push_back(sent);
    pending_[sent.reqId] = std::move(sent);
}

void CoapFake::responseSent(Sent sent) {
    responses_.push_back(std::move(sent));
}

bool CoapFake::takeIncomingRequest(int reqId) {
    return incoming_.erase(reqId) > 0;
}

void CoapFake::requestCancelled(int reqId) {
    cancelled_.push_back(reqId);
    pending_.erase(reqId);
}

} // namespace particle::test

using particle::test::CoapFake;

int coap_add_connection_handler(coap_connection_callback cb, void* arg, void* reserved) {
    CoapFake::instance().connHandlers.push_back({ cb, arg });
    return 0;
}

void coap_remove_connection_handler(coap_connection_callback cb, void* reserved) {
    auto& h = CoapFake::instance().connHandlers;
    h.erase(std::remove_if(h.begin(), h.end(), [cb](const auto& e) { return e.cb == cb; }), h.end());
}

int coap_add_request_handler(const char* path, int method, int flags, coap_request_callback cb, void* arg, void* reserved) {
    CoapFake::instance().reqHandlers.push_back({ path, method, cb, arg });
    return 0;
}

void coap_remove_request_handler(const char* path, int method, void* reserved) {
    auto& h = CoapFake::instance().reqHandlers;
    h.erase(std::remove_if(h.begin(), h.end(), [&](const auto& e) {
        return e.uri == path && e.method == method;
    }), h.end());
}

int coap_begin_request(coap_message** msg, const char* path, int method, int timeout, int flags, void* reserved) {
    auto& fake = CoapFake::instance();
    if (!fake.isConnected()) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    auto m = new coap_message();
    m->uri = path;
    m->method = method;
    m->reqId = fake.nextReqId();
    *msg = m;
    return m->reqId;
}

int coap_end_request(coap_message* msg, coap_response_callback resp_cb, coap_ack_callback ack_cb,
        coap_error_callback error_cb, void* arg, void* reserved) {
    CoapFake::Sent s = {};
    s.reqId = msg->reqId;
    s.isResponse = false;
    s.uri = msg->uri;
    s.method = msg->method;
    s.payload = msg->payload;
    s.respCb = resp_cb;
    s.errorCb = error_cb;
    s.arg = arg;
    delete msg; // The message is owned by the API after this call
    CoapFake::instance().requestSent(std::move(s));
    return 0;
}

int coap_begin_response(coap_message** msg, int status, int req_id, int flags, void* reserved) {
    auto& fake = CoapFake::instance();
    if (!fake.isConnected()) {
        return SYSTEM_ERROR_COAP_CONNECTION_CLOSED;
    }
    if (!fake.takeIncomingRequest(req_id)) {
        return SYSTEM_ERROR_COAP_REQUEST_NOT_FOUND;
    }
    auto m = new coap_message();
    m->status = status;
    m->reqId = req_id;
    m->isResponse = true;
    *msg = m;
    return 0;
}

int coap_end_response(coap_message* msg, coap_ack_callback ack_cb, coap_error_callback error_cb, void* arg,
        void* reserved) {
    CoapFake::Sent s = {};
    s.reqId = msg->reqId;
    s.isResponse = true;
    s.status = msg->status;
    s.payload = msg->payload;
    s.errorCb = error_cb;
    s.arg = arg;
    delete msg;
    CoapFake::instance().responseSent(std::move(s));
    return 0;
}

void coap_destroy_message(coap_message* msg, void* reserved) {
    delete msg;
}

int coap_cancel_request(int req_id, void* reserved) {
    CoapFake::instance().requestCancelled(req_id);
    return 0;
}

int coap_write_block(coap_message* msg, const char* data, size_t* size, coap_block_callback block_cb,
        coap_error_callback error_cb, void* arg, void* reserved) {
    if (msg->payload.size() + *size > COAP_MAX_PAYLOAD_SIZE) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    msg->payload.append(data, *size);
    return 0;
}

int coap_read_block(coap_message* msg, char* data, size_t* size, coap_block_callback block_cb,
        coap_error_callback error_cb, void* arg, void* reserved) {
    if (*size > 0) {
        if (msg->readPos >= msg->payload.size()) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        *size = std::min(*size, msg->payload.size() - msg->readPos);
        if (data) {
            std::memcpy(data, msg->payload.data() + msg->readPos, *size);
        }
        msg->readPos += *size;
    }
    return 0;
}

int coap_peek_block(coap_message* msg, char* data, size_t size, void* reserved) {
    if (size > 0) {
        if (msg->readPos >= msg->payload.size()) {
            return SYSTEM_ERROR_END_OF_STREAM;
        }
        size = std::min(size, msg->payload.size() - msg->readPos);
        if (data) {
            std::memcpy(data, msg->payload.data() + msg->readPos, size);
        }
    }
    return size;
}
