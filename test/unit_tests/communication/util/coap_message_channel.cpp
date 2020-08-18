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

#include "coap_message_channel.h"

namespace particle::protocol::test {

ProtocolError CoapMessageChannel::receive(Message& msg) {
    if (!send_.empty()) {
        const auto m = send_.front();
        send_.pop();
        const auto s = m.encode();
        if (s.size() > sizeof(queue)) {
            return ProtocolError::INSUFFICIENT_STORAGE;
        }
        memcpy(queue, s.data(), s.size());
        msg = Message(queue, sizeof(queue), s.size());
    } else {
        msg = Message();
    }
    return ProtocolError::NO_ERROR;
}

} // namespace particle::protocol::test
