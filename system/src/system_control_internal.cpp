/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
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

#include "system_control_internal.h"

#include "system_network_internal.h"
#include "spark_wiring_system.h"
#include "debug.h"

#if SYSTEM_CONTROL_ENABLED

namespace {

using namespace particle;

SystemControl g_systemControl;

} // namespace

particle::SystemControl::SystemControl() :
        usbReqChannel_(this),
        appReqHandler_(nullptr) {
}

void particle::SystemControl::processRequest(ctrl_request* req, ControlRequestChannel* /* channel */) {
    switch (req->type) {
    case CTRL_REQUEST_RESET: {
        System.reset();
        setResult(req, SYSTEM_ERROR_NONE);
        break;
    }
    case CTRL_REQUEST_DFU_MODE: {
        System.dfu(false);
        setResult(req, SYSTEM_ERROR_NONE);
        break;
    }
    case CTRL_REQUEST_SAFE_MODE: {
        System.enterSafeMode();
        setResult(req, SYSTEM_ERROR_NONE);
        break;
    }
    case CTRL_REQUEST_LISTENING_MODE: {
        // FIXME: Any non-empty request is interpreted as the "stop" command
        const bool stop = (req->request_size > 0);
        network.listen(stop);
        setResult(req, SYSTEM_ERROR_NONE);
        break;
    }
    case CTRL_REQUEST_MODULE_INFO: {
        // FIXME
        break;
    }
    default:
        // Forward the request to the application thread
        if (appReqHandler_) {
            appReqHandler_(req); // FIXME
        } else {
            setResult(req, SYSTEM_ERROR_NOT_SUPPORTED);
        }
        break;
    }
}

particle::SystemControl* particle::SystemControl::instance() {
    return &g_systemControl;
}

// System API
int system_ctrl_set_app_request_handler(ctrl_request_handler_fn handler, void* reserved) {
    return SystemControl::instance()->setAppRequestHandler(handler);
}

void system_ctrl_free_request_data(ctrl_request* req, void* reserved) {
    SystemControl::instance()->freeRequestData(req);
}

int system_ctrl_alloc_reply_data(ctrl_request* req, size_t size, void* reserved) {
    return SystemControl::instance()->allocReplyData(req, size);
}

void system_ctrl_set_result(ctrl_request* req, int result, void* reserved) {
    SystemControl::instance()->setResult(req, (system_error_t)result);
}

#else // SYSTEM_CONTROL_ENABLED

// System API
int system_ctrl_set_app_request_handler(ctrl_request_handler_fn handler, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

void system_ctrl_free_request_data(ctrl_request* req, void* reserved) {
}

int system_ctrl_alloc_reply_data(ctrl_request* req, size_t size, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

void system_ctrl_set_result(ctrl_request* req, int result, void* reserved) {
}

#endif // !SYSTEM_CONTROL_ENABLED

#ifdef USB_VENDOR_REQUEST_ENABLE

// These functions are deprecated and exported only for compatibility
void system_set_usb_request_app_handler(void*, void*) {
}

void system_set_usb_request_result(void*, int, void*) {
}

#endif // defined(USB_VENDOR_REQUEST_ENABLE)
