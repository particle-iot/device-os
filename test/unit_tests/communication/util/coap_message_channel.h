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

#include "coap_message.h"

#include "buffer_message_channel.h"
#include "protocol_defs.h"

#include <memory>
#include <queue>

namespace particle {

namespace protocol {

namespace test {

class CoapMessageChannel: public BufferMessageChannel<PROTOCOL_BUFFER_SIZE> {
public:
    CoapMessageChannel();

    // Sends a message to the device
    CoapMessageChannel& sendMessage(CoapMessage msg);
    // Receives a message from the device
    CoapMessage receiveMessage();
    // Skips N messages received from the device
    CoapMessageChannel& skipMessages(unsigned count);
    // Returns true if there's a message received from the device
    bool hasMessages() const;

    // Reimplemented from AbstractMessageChannel
    ProtocolError send(Message& msg) override;
    ProtocolError receive(Message& msg) override;
    ProtocolError command(Command cmd, void* arg) override;
    bool is_unreliable() override;
    ProtocolError establish() override;
    ProtocolError notify_established() override;
    void notify_client_messages_processed() override;
    AppStateDescriptor cached_app_state_descriptor() const override;
    void reset() override;

private:
    std::queue<CoapMessage> send_;
    std::queue<CoapMessage> recv_;
    CoapMessageId lastMsgId_;
};

inline CoapMessageChannel::CoapMessageChannel() :
        lastMsgId_(0) {
}

inline CoapMessageChannel& CoapMessageChannel::sendMessage(CoapMessage msg) {
    send_.push(std::move(msg));
    return *this;
}

inline CoapMessage CoapMessageChannel::receiveMessage() {
    if (recv_.empty()) {
        throw std::runtime_error("Message queue is empty");
    }
    const CoapMessage msg = std::move(recv_.front());
    recv_.pop();
    return msg;
}

inline CoapMessageChannel& CoapMessageChannel::skipMessages(unsigned count) {
    for (unsigned i = 0; i < count; ++i) {
        if (recv_.empty()) {
            throw std::runtime_error("Message queue is empty");
        }
        recv_.pop();
    }
    return *this;
}

inline bool CoapMessageChannel::hasMessages() const {
    return !recv_.empty();
}

inline ProtocolError CoapMessageChannel::establish() {
    return ProtocolError::NO_ERROR;
}

inline ProtocolError CoapMessageChannel::command(Command cmd, void* arg) {
    return ProtocolError::NO_ERROR;
}

inline bool CoapMessageChannel::is_unreliable() {
    return true;
}

inline ProtocolError CoapMessageChannel::notify_established() {
    return ProtocolError::NO_ERROR;
}

inline void CoapMessageChannel::notify_client_messages_processed() {
}

inline AppStateDescriptor CoapMessageChannel::cached_app_state_descriptor() const {
    return AppStateDescriptor();
}

inline void CoapMessageChannel::reset() {
}

} // namespace test

} // namespace protocol

} // namespace particle
