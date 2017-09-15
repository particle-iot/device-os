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

#include "system_update.h"
#include "appender.h"

#if SYSTEM_CONTROL_ENABLED

LOG_SOURCE_CATEGORY("system.ctrl");

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
        const size_t bufSize = 1024;
        if (allocReplyData(req, bufSize) != 0) {
            setResult(req, SYSTEM_ERROR_NO_MEMORY);
            return;
        }
        BufferAppender appender((uint8_t*)req->reply_data, req->reply_size);
        if (!system_module_info(append_instance, &appender)) {
            freeReplyData(req);
            setResult(req, SYSTEM_ERROR_UNKNOWN);
            return;
        }
        req->reply_size = appender.size();
        setResult(req, SYSTEM_ERROR_NONE);
        break;
    }
    default:
        // Forward the request to the application thread
        if (appReqHandler_) {
            processAppRequest(req);
        } else {
            setResult(req, SYSTEM_ERROR_NOT_SUPPORTED);
        }
        break;
    }
}

void particle::SystemControl::processAppRequest(ctrl_request* req) {
    // FIXME: Request leak may occur if underlying asynchronous event cannot be queued
    APPLICATION_THREAD_CONTEXT_ASYNC(processAppRequest(req));
    SPARK_ASSERT(appReqHandler_); // Checked in processRequest()
    appReqHandler_(req);
}

particle::SystemControl* particle::SystemControl::instance() {
    return &g_systemControl;
}

// System API
int system_ctrl_set_app_request_handler(ctrl_request_handler_fn handler, void* reserved) {
    return SystemControl::instance()->setAppRequestHandler(handler);
}

int system_ctrl_alloc_reply_data(ctrl_request* req, size_t size, void* reserved) {
    return SystemControl::instance()->allocReplyData(req, size);
}

void system_ctrl_free_reply_data(ctrl_request* req, void* reserved) {
    SystemControl::instance()->freeReplyData(req);
}

void system_ctrl_free_request_data(ctrl_request* req, void* reserved) {
    SystemControl::instance()->freeRequestData(req);
}

void system_ctrl_set_result(ctrl_request* req, int result, void* reserved) {
    SystemControl::instance()->setResult(req, (system_error_t)result);
}

#else // SYSTEM_CONTROL_ENABLED

// System API
int system_ctrl_set_app_request_handler(ctrl_request_handler_fn handler, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int system_ctrl_alloc_reply_data(ctrl_request* req, size_t size, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

void system_ctrl_free_reply_data(ctrl_request* req, void* reserved) {
}

void system_ctrl_free_request_data(ctrl_request* req, void* reserved) {
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
