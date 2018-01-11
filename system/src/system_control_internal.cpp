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

#if SYSTEM_CONTROL_ENABLED

#include "system_network_internal.h"
#include "system_update.h"
#include "spark_wiring_system.h"
#include "appender.h"
#include "debug.h"

#include "control/network.h"
#include "control/wifi.h"
#include "control/config.h"
#include "control/storage.h"

namespace {

using namespace particle;

typedef int(*ReplyFormatterCallback)(Appender*, void* data);

int formatReplyData(ctrl_request* req, ReplyFormatterCallback callback, void* data = nullptr,
        size_t maxSize = std::numeric_limits<size_t>::max()) {
    size_t bufSize = std::min((size_t)128, maxSize); // Initial size of the reply buffer
    for (;;) {
        int ret = system_ctrl_alloc_reply_data(req, bufSize, nullptr);
        if (ret != 0) {
            system_ctrl_alloc_reply_data(req, 0, nullptr);
            return ret;
        }
        BufferAppender2 appender(req->reply_data, bufSize);
        ret = callback(&appender, data);
        if (ret != 0) {
            system_ctrl_alloc_reply_data(req, 0, nullptr);
            return ret;
        }
        const size_t size = appender.dataSize();
        if (size > maxSize) {
            system_ctrl_alloc_reply_data(req, 0, nullptr);
            return SYSTEM_ERROR_TOO_LARGE;
        }
        if (size > bufSize) {
            // Increase the buffer size and format the data once again
            bufSize = std::min(size + size / 16, maxSize);
            continue;
        }
        req->reply_size = size;
        return 0; // OK
    }
}

SystemControl g_systemControl;

} // namespace

particle::SystemControl::SystemControl() :
        usbReqChannel_(this),
        appReqHandler_(nullptr) {
}

void particle::SystemControl::processRequest(ctrl_request* req, ControlRequestChannel* /* channel */) {
    switch (req->type) {
    case CTRL_REQUEST_RESET: {
        setResult(req, SYSTEM_ERROR_NONE, [](int result, void* data) {
            System.reset();
        });
        break;
    }
    case CTRL_REQUEST_FACTORY_RESET: {
        setResult(req, SYSTEM_ERROR_NONE, [](int result, void* data) {
            System.factoryReset();
        });
        break;
    }
    case CTRL_REQUEST_DFU_MODE: {
        setResult(req, SYSTEM_ERROR_NONE, [](int result, void* data) {
            System.dfu(false);
        });
        break;
    }
    case CTRL_REQUEST_SAFE_MODE: {
        setResult(req, SYSTEM_ERROR_NONE, [](int result, void* data) {
            System.enterSafeMode();
        });
        break;
    }
    case CTRL_REQUEST_START_LISTENING: {
        network.listen();
        setResult(req, SYSTEM_ERROR_NONE);
        break;
    }
    case CTRL_REQUEST_STOP_LISTENING: {
        network.listen(true /* stop */);
        setResult(req, SYSTEM_ERROR_NONE);
        break;
    }
    case CTRL_REQUEST_MODULE_INFO: {
        struct Formatter {
            static int callback(Appender* appender, void* data) {
                if (!appender->append('{') ||
                        !system_module_info(append_instance, appender) ||
                        !appender->append('}')) {
                    return SYSTEM_ERROR_UNKNOWN;
                }
                return 0;
            }
        };
        const int ret = formatReplyData(req, Formatter::callback);
        setResult(req, ret);
        break;
    }
    case CTRL_REQUEST_DIAGNOSTIC_INFO: {
        if (req->request_size > 0) {
            // TODO: Querying a part of the diagnostic data is not supported
            setResult(req, SYSTEM_ERROR_NOT_SUPPORTED);
        } else {
            struct Formatter {
                static int callback(Appender* appender, void* data) {
                    return system_format_diag_data(nullptr, 0, 0, append_instance, appender, nullptr);
                }
            };
            const int ret = formatReplyData(req, Formatter::callback);
            setResult(req, ret);
        }
        break;
    }
#if Wiring_WiFi == 1
    /* wifi requests */
    case CTRL_REQUEST_WIFI_GET_ANTENNA: {
        setResult(req, control::wifi::handleGetAntennaRequest(req));
        break;
    }
    case CTRL_REQUEST_WIFI_SET_ANTENNA: {
        setResult(req, control::wifi::handleSetAntennaRequest(req));
        break;
    }
    case CTRL_REQUEST_WIFI_SCAN: {
        setResult(req, control::wifi::handleScanRequest(req));
        break;
    }
    case CTRL_REQUEST_WIFI_GET_CREDENTIALS: {
        setResult(req, control::wifi::handleGetCredentialsRequest(req));
        break;
    }
    case CTRL_REQUEST_WIFI_SET_CREDENTIALS: {
        setResult(req, control::wifi::handleSetCredentialsRequest(req));
        break;
    }
    case CTRL_REQUEST_WIFI_CLEAR_CREDENTIALS: {
        setResult(req, control::wifi::handleClearCredentialsRequest(req));
        break;
    }
#endif // Wiring_WiFi
    /* network requests */
    case CTRL_REQUEST_NETWORK_GET_CONFIGURATION: {
        setResult(req, control::network::handleGetConfigurationRequest(req));
        break;
    }
    case CTRL_REQUEST_NETWORK_GET_STATUS: {
        setResult(req, control::network::handleGetStatusRequest(req));
        break;
    }
    case CTRL_REQUEST_NETWORK_SET_CONFIGURATION: {
        setResult(req, control::network::handleSetConfigurationRequest(req));
        break;
    }
    /* config requests */
    case CTRL_REQUEST_SET_CLAIM_CODE: {
        setResult(req, control::config::handleSetClaimCodeRequest(req));
        break;
    }
    case CTRL_REQUEST_IS_CLAIMED: {
        setResult(req, control::config::handleIsClaimedRequest(req));
        break;
    }
    case CTRL_REQUEST_SET_SECURITY_KEY: {
        setResult(req, control::config::handleSetSecurityKeyRequest(req));
        break;
    }
    case CTRL_REQUEST_GET_SECURITY_KEY: {
        setResult(req, control::config::handleGetSecurityKeyRequest(req));
        break;
    }
    case CTRL_REQUEST_SET_SERVER_ADDRESS: {
        setResult(req, control::config::handleSetServerAddressRequest(req));
        break;
    }
    case CTRL_REQUEST_GET_SERVER_ADDRESS: {
        setResult(req, control::config::handleGetServerAddressRequest(req));
        break;
    }
    case CTRL_REQUEST_SET_SERVER_PROTOCOL: {
        setResult(req, control::config::handleSetServerProtocolRequest(req));
        break;
    }
    case CTRL_REQUEST_GET_SERVER_PROTOCOL: {
        setResult(req, control::config::handleGetServerProtocolRequest(req));
        break;
    }
    case CTRL_REQUEST_START_NYAN_SIGNAL: {
        setResult(req, control::config::handleStartNyanRequest(req));
        break;
    }
    case CTRL_REQUEST_STOP_NYAN_SIGNAL: {
        setResult(req, control::config::handleStopNyanRequest(req));
        break;
    }
    case CTRL_REQUEST_SET_SOFTAP_SSID: {
        setResult(req, control::config::handleSetSoftapSsidRequest(req));
        break;
    }
    case CTRL_REQUEST_START_FIRMWARE_UPDATE: {
        setResult(req, control::startFirmwareUpdateRequest(req));
        break;
    }
    case CTRL_REQUEST_FINISH_FIRMWARE_UPDATE: {
        control::finishFirmwareUpdateRequest(req);
        break;
    }
    case CTRL_REQUEST_CANCEL_FIRMWARE_UPDATE: {
        setResult(req, control::cancelFirmwareUpdateRequest(req));
        break;
    }
    case CTRL_REQUEST_FIRMWARE_UPDATE_DATA: {
        setResult(req, control::firmwareUpdateDataRequest(req));
        break;
    }
    case CTRL_REQUEST_DESCRIBE_STORAGE: {
        setResult(req, control::describeStorageRequest(req));
        break;
    }
    case CTRL_REQUEST_READ_SECTION_DATA: {
        setResult(req, control::readSectionDataRequest(req));
        break;
    }
    case CTRL_REQUEST_WRITE_SECTION_DATA: {
        setResult(req, control::writeSectionDataRequest(req));
        break;
    }
    case CTRL_REQUEST_CLEAR_SECTION_DATA: {
        setResult(req, control::clearSectionDataRequest(req));
        break;
    }
    case CTRL_REQUEST_GET_SECTION_DATA_SIZE: {
        setResult(req, control::getSectionDataSizeRequest(req));
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

void system_ctrl_free_request_data(ctrl_request* req, void* reserved) {
    SystemControl::instance()->freeRequestData(req);
}

void system_ctrl_set_result(ctrl_request* req, int result, ctrl_completion_handler_fn handler, void* data, void* reserved) {
    SystemControl::instance()->setResult(req, result, handler, data);
}

#else // !SYSTEM_CONTROL_ENABLED

// System API
int system_ctrl_set_app_request_handler(ctrl_request_handler_fn handler, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int system_ctrl_alloc_reply_data(ctrl_request* req, size_t size, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

void system_ctrl_free_request_data(ctrl_request* req, void* reserved) {
}

void system_ctrl_set_result(ctrl_request* req, int result, ctrl_completion_handler_fn handler, void* data, void* reserved) {
}

#endif // !SYSTEM_CONTROL_ENABLED

#ifdef USB_VENDOR_REQUEST_ENABLE

// These functions are deprecated and exported only for compatibility
void system_set_usb_request_app_handler(void*, void*) {
}

void system_set_usb_request_result(void*, int, void*) {
}

#endif // defined(USB_VENDOR_REQUEST_ENABLE)
