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

#include "coap_message_channel.h"
#include "descriptor_callbacks.h"
#include "protocol_callbacks.h"

#include "protocol.h"

namespace particle {

namespace protocol {

namespace test {

class ProtocolStub: public Protocol {
public:
    explicit ProtocolStub(CoapMessageChannel* channel);

    DescriptorCallbacks* descriptor();
    ProtocolCallbacks* callbacks();
    CoapMessageChannel* channel();

    // Reimplemented from Protocol
    void init(const char* id, const SparkKeys& keys, const SparkCallbacks& cb, const SparkDescriptor& desc) override;
    int command(ProtocolCommands::Enum cmd, uint32_t val, const void* data) override;
    size_t build_hello(Message& msg, uint16_t flags) override;

private:
    DescriptorCallbacks desc_;
    ProtocolCallbacks cb_;
    CoapMessageChannel* channel_;
};

inline DescriptorCallbacks* ProtocolStub::descriptor() {
    return &desc_;
}

inline ProtocolCallbacks* ProtocolStub::callbacks() {
    return &cb_;
}

inline CoapMessageChannel* ProtocolStub::channel() {
    return channel_;
}

inline void ProtocolStub::init(const char* id, const SparkKeys& keys, const SparkCallbacks& cb, const SparkDescriptor& desc) {
}

inline int ProtocolStub::command(ProtocolCommands::Enum cmd, uint32_t val, const void* data) {
    return 0;
}

inline size_t ProtocolStub::build_hello(Message& msg, uint16_t flags) {
    return 0;
}

} // namespace test

} // namespace protocol

} // namespace particle
