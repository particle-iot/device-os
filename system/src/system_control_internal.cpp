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

#include "system_threading.h"
#include "system_network.h"
#include "system_network_internal.h"
#include "system_update.h"
#include "spark_wiring_system.h"
#include "appender.h"
#include "debug.h"
#include "delay_hal.h"
#include "hal_platform.h"

#include "control/network.h"
#include "control/wifi_new.h"
#include "control/cellular.h"
#include "control/config.h"
#include "control/storage.h"
#include "control/cloud.h"

namespace particle {

namespace system {

namespace {

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
        BufferAppender appender(req->reply_data, bufSize);
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

} // particle::system::

SystemControl::SystemControl() :
#ifdef USB_VENDOR_REQUEST_ENABLE
        usbChannel_(this),
#endif
#if HAL_PLATFORM_BLE
        bleChannel_(this),
#endif
        appReqHandler_(nullptr) {
}

int SystemControl::init() {
#if HAL_PLATFORM_BLE
    // BleControlRequestChannel::init() should be called only once
    // as characteristic UUIDs are currently called only once. If BLE prov mode
    // is enabled, we need to make sure to call characteristic UUIDs in prov
    // mode system calls for the first time instead of here.
    // See system_ble_prov_mode()
    if (system_mode() == SAFE_MODE || !HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
        const int ret = bleChannel_.init();
        if (ret != 0) {
            return ret;
        }
    }
#endif
    return 0;
}

void SystemControl::run() {
#if HAL_PLATFORM_BLE
    bleChannel_.run();
#endif
}

void SystemControl::processRequest(ctrl_request* req, ControlRequestChannel* /* channel */) {

    // Filter requests based on the setControlRequestFilter system API
    system_ctrl_filter* filter = getFiltersList();

    while(filter != nullptr) {
        if (filter->req == req->type) {
            break;
        }
        filter = filter->next;
    }

    auto action = filter ? filter->action : getDefaultFilterAction();

    if (action == SYSTEM_CTRL_ACL_DENY) {
        setResult(req, SYSTEM_ERROR_NOT_ALLOWED);
        return;
    }

    switch (req->type) {
    case CTRL_REQUEST_DEVICE_ID: {
        setResult(req, control::config::getDeviceId(req));
        break;
    }
    case CTRL_REQUEST_SERIAL_NUMBER: {
        setResult(req, control::config::getSerialNumber(req));
        break;
    }
    case CTRL_REQUEST_SYSTEM_VERSION: {
        setResult(req, control::config::getSystemVersion(req));
        break;
    }
    case CTRL_REQUEST_NCP_FIRMWARE_VERSION: {
        setResult(req, control::config::getNcpFirmwareVersion(req));
        break;
    }
    case CTRL_REQUEST_GET_SYSTEM_CAPABILITIES: {
        setResult(req, control::config::getSystemCapabilities(req));
        break;
    }
    case CTRL_REQUEST_SET_FEATURE: {
        setResult(req, control::config::setFeature(req));
        break;
    }
    case CTRL_REQUEST_GET_FEATURE: {
        setResult(req, control::config::getFeature(req));
        break;
    }
    case CTRL_REQUEST_RESET: {
        setResult(req, SYSTEM_ERROR_NONE, [](int result, void* data) {
            HAL_Delay_Milliseconds(1000);
            System.reset();
        });
        break;
    }
    case CTRL_REQUEST_FACTORY_RESET: {
        setResult(req, SYSTEM_ERROR_NONE, [](int result, void* data) {
            HAL_Delay_Milliseconds(1000);
            System.factoryReset();
        });
        break;
    }
    case CTRL_REQUEST_DFU_MODE: {
        setResult(req, SYSTEM_ERROR_NONE, [](int result, void* data) {
            HAL_Delay_Milliseconds(1000);
            System.dfu(false);
        });
        break;
    }
    case CTRL_REQUEST_SAFE_MODE: {
        setResult(req, SYSTEM_ERROR_NONE, [](int result, void* data) {
            HAL_Delay_Milliseconds(1000);
            System.enterSafeMode();
        });
        break;
    }
    case CTRL_REQUEST_START_LISTENING: {
        setResult(req, network_listen_sync(0, 0, 0));
        break;
    }
    case CTRL_REQUEST_STOP_LISTENING: {
        network_listen(0, NETWORK_LISTEN_EXIT, 0);
        setResult(req, SYSTEM_ERROR_NONE);
        break;
    }
    case CTRL_REQUEST_GET_DEVICE_MODE: {
        setResult(req, control::config::getDeviceMode(req));
        break;
    }
    case CTRL_REQUEST_SET_DEVICE_SETUP_DONE: {
        setResult(req, control::config::setDeviceSetupDone(req));
        break;
    }
    case CTRL_REQUEST_IS_DEVICE_SETUP_DONE: {
        setResult(req, control::config::isDeviceSetupDone(req));
        break;
    }
    case CTRL_REQUEST_SET_STARTUP_MODE: {
        setResult(req, control::config::setStartupMode(req));
        break;
    }
    case CTRL_REQUEST_ECHO: {
        setResult(req, control::config::echo(req));
        break;
    }
    case CTRL_REQUEST_GET_MODULE_INFO: {
        setResult(req, control::getModuleInfo(req));
        break;
    }
    case CTRL_REQUEST_DIAGNOSTIC_INFO: {
        if (req->request_size > 0) {
            // TODO: Querying a part of the diagnostic data is not supported
            setResult(req, SYSTEM_ERROR_NOT_SUPPORTED);
        } else {
            struct Formatter {
                static int callback(Appender* appender, void* data) {
                    return system_format_diag_data(nullptr, 0, 0, Appender::callback, appender, nullptr);
                }
            };
            const int ret = formatReplyData(req, Formatter::callback);
            setResult(req, ret);
        }
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
    case CTRL_REQUEST_CLOUD_GET_CONNECTION_STATUS: {
        setResult(req, ctrl::cloud::getConnectionStatus(req));
        break;
    }
    case CTRL_REQUEST_CLOUD_CONNECT: {
        setResult(req, ctrl::cloud::connect(req));
        break;
    }
    case CTRL_REQUEST_CLOUD_DISCONNECT: {
        setResult(req, ctrl::cloud::disconnect(req));
        break;
    }
#if HAL_USE_SOCKET_HAL_POSIX && HAL_PLATFORM_IFAPI
    case CTRL_REQUEST_NETWORK_GET_INTERFACE_LIST: {
        setResult(req, control::network::getInterfaceList(req));
        break;
    }
    case CTRL_REQUEST_NETWORK_GET_INTERFACE: {
        setResult(req, control::network::getInterface(req));
        break;
    }
#endif // HAL_USE_SOCKET_HAL_POSIX && HAL_PLATFORM_IFAPI
#if HAL_PLATFORM_NCP && HAL_PLATFORM_WIFI
    case CTRL_REQUEST_WIFI_JOIN_NEW_NETWORK: {
        setResult(req, ctrl::wifi::joinNewNetwork(req));
        break;
    }
    case CTRL_REQUEST_WIFI_JOIN_KNOWN_NETWORK: {
        setResult(req, ctrl::wifi::joinKnownNetwork(req));
        break;
    }
    case CTRL_REQUEST_WIFI_GET_KNOWN_NETWORKS: {
        setResult(req, ctrl::wifi::getKnownNetworks(req));
        break;
    }
    case CTRL_REQUEST_WIFI_REMOVE_KNOWN_NETWORK: {
        setResult(req, ctrl::wifi::removeKnownNetwork(req));
        break;
    }
    case CTRL_REQUEST_WIFI_CLEAR_KNOWN_NETWORKS: {
        setResult(req, ctrl::wifi::clearKnownNetworks(req));
        break;
    }
    case CTRL_REQUEST_WIFI_GET_CURRENT_NETWORK: {
        setResult(req, ctrl::wifi::getCurrentNetwork(req));
        break;
    }
    case CTRL_REQUEST_WIFI_SCAN_NETWORKS: {
        setResult(req, ctrl::wifi::scanNetworks(req));
        break;
    }
#endif // HAL_PLATFORM_NCP && HAL_PLATFORM_WIFI
#if HAL_PLATFORM_NCP && HAL_PLATFORM_CELLULAR
    case CTRL_REQUEST_CELLULAR_SET_ACCESS_POINT: {
        setResult(req, ctrl::cellular::setAccessPoint(req));
        break;
    }
    case CTRL_REQUEST_CELLULAR_GET_ACCESS_POINT: {
        setResult(req, ctrl::cellular::getAccessPoint(req));
        break;
    }
    case CTRL_REQUEST_CELLULAR_SET_ACTIVE_SIM: {
        setResult(req, ctrl::cellular::setActiveSim(req));
        break;
    }
    case CTRL_REQUEST_CELLULAR_GET_ACTIVE_SIM: {
        setResult(req, ctrl::cellular::getActiveSim(req));
        break;
    }
    case CTRL_REQUEST_CELLULAR_GET_ICCID: {
        setResult(req, ctrl::cellular::getIccid(req));
        break;
    }
#endif // HAL_PLATFORM_NCP && HAL_PLATFORM_CELLULAR
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

void SystemControl::processAppRequest(ctrl_request* req) {
    // FIXME: Request leak may occur if underlying asynchronous event cannot be queued
    APPLICATION_THREAD_CONTEXT_ASYNC(processAppRequest(req));
    SPARK_ASSERT(appReqHandler_); // Checked in processRequest()
    appReqHandler_(req);
}

SystemControl* SystemControl::instance() {
    return &g_systemControl;
}

} // particle::system

} // particle

// System API
int system_ctrl_set_app_request_handler(ctrl_request_handler_fn handler, void* reserved) {
    return particle::system::SystemControl::instance()->setAppRequestHandler(handler);
}

int system_ctrl_alloc_reply_data(ctrl_request* req, size_t size, void* reserved) {
    return particle::system::SystemControl::instance()->allocReplyData(req, size);
}

void system_ctrl_free_request_data(ctrl_request* req, void* reserved) {
    particle::system::SystemControl::instance()->freeRequestData(req);
}

void system_ctrl_set_result(ctrl_request* req, int result, ctrl_completion_handler_fn handler, void* data, void* reserved) {
    particle::system::SystemControl::instance()->setResult(req, result, handler, data);
}

int system_ctrl_set_request_filter(system_ctrl_acl default_action, system_ctrl_filter* filters, void* reserved) {

    SYSTEM_THREAD_CONTEXT_SYNC(system_ctrl_set_request_filter(default_action, filters, reserved));

    particle::system::SystemControl::instance()->setDefaultFilterAction(default_action);
    particle::system::SystemControl::instance()->setFiltersList(filters);

    return 0;
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

int system_ctrl_set_request_filter(system_ctrl_acl default_action, system_ctrl_filter* filters, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

#endif // !SYSTEM_CONTROL_ENABLED

#ifdef USB_VENDOR_REQUEST_ENABLE

// These functions are deprecated and exported only for compatibility
void system_set_usb_request_app_handler(void*, void*) {
}

void system_set_usb_request_result(void*, int, void*) {
}

#endif // defined(USB_VENDOR_REQUEST_ENABLE)
