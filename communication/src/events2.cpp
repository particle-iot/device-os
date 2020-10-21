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

#include "events2.h"

#include "system_error.h"

#include <utility>

namespace particle {

namespace protocol {

using std::swap; // For ADL

void Events::reset(bool clearSubscriptions, bool invokeCallbacks) {
    Vector<Event> inEvents;
    swap(inEvents_, inEvents);
    Vector<Event> outEvents;
    swap(outEvents_, inEvents);
    if (clearSubscriptions) {
        subscr_.clear();
        subscrChecksum_ = 0;
    }
    // Note: Do not reinitialize lastEventHandle_. Event handles must be unique across different sessions
    if (invokeCallbacks) {
        for (const auto& e: inEvents) {
            if (e.statusFn) {
                e.statusFn(e.handle, PROTOCOL_EVENT_STATUS_ERROR, SYSTEM_ERROR_CANCELLED, e.userData);
            }
        }
        for (const auto& e: outEvents) {
            if (e.statusFn) {
                e.statusFn(e.handle, PROTOCOL_EVENT_STATUS_ERROR, SYSTEM_ERROR_CANCELLED, e.userData);
            }
        }
    }
}

} // namespace protocol

} // namespace particle
