/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include "system_control.h"

namespace particle {

// Handler for test runner requests
class RequestHandler {
public:
    RequestHandler();
    ~RequestHandler();

    int init();
    void destroy();

    void process(ctrl_request* ctrlReq);

    static RequestHandler* instance();

private:
    class Request;

    bool inited_;

    int request(ctrl_request* ctrlReq);
    int request(Request* req);
    int initSuite(Request* req);
    int listTests(Request* req);
    int startTest(Request* req);
    int getStatus(Request* req);
    int getLog(Request* req);
    int reset(Request* req);
    int readMailbox(Request* req);
};

} // namespace particle
