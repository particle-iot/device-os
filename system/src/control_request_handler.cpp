/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "control_request_handler.h"

#include <cstdlib>

namespace particle {

int ControlRequestChannel::allocReplyData(ctrl_request* req, size_t size) {
    if (size > 0) {
        const auto data = (char*)realloc(req->reply_data, size);
        if (!data) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        req->reply_data = data;
    } else {
        free(req->reply_data);
        req->reply_data = nullptr;
    }
    req->reply_size = size;
    return 0;
}

void ControlRequestChannel::freeRequestData(ctrl_request* req) {
    free(req->request_data);
    req->request_data = nullptr;
    req->request_size = 0;
}

} // particle
