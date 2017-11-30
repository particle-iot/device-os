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

#include "ota_flash_hal_stm32f2xx.h"

#include "spark_wiring_system.h"

#include "appender.h"
#include "debug.h"

#include <algorithm>
#include <limits>
#include <cstdio>

#include "nanopb_misc.h"
#include "control/common.pb.h"
#include "control/control.pb.h"
#include "control/config.pb.h"

namespace {

using namespace particle;

typedef int(*ReplyFormatterCallback)(Appender*, void* data);

int formatReplyData(ctrl_request* req, ReplyFormatterCallback callback, void* data = nullptr,
        size_t maxSize = std::numeric_limits<size_t>::max()) {
    size_t bufSize = std::min((size_t)128, maxSize); // Initial size of the reply buffer
    for (;;) {
        int ret = system_ctrl_alloc_reply_data(req, bufSize, nullptr);
        if (ret != 0) {
            system_ctrl_free_reply_data(req, nullptr);
            return ret;
        }
        BufferAppender2 appender(req->reply_data, bufSize);
        ret = callback(&appender, data);
        if (ret != 0) {
            system_ctrl_free_reply_data(req, nullptr);
            return ret;
        }
        const size_t size = appender.dataSize();
        if (size > maxSize) {
            system_ctrl_free_reply_data(req, nullptr);
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

int encodeReplyMessage(ctrl_request* req, const pb_field_t* fields, const void* src) {
    pb_ostream_t* stream = nullptr;
    size_t sz = 0;
    int ret = SYSTEM_ERROR_UNKNOWN;

    // Calculate size
    bool res = pb_get_encoded_size(&sz, fields, src);
    if (!res) {
        goto cleanup;
    }

    // Allocate reply data
    ret = system_ctrl_alloc_reply_data(req, sz, nullptr);
    if (ret != SYSTEM_ERROR_NONE) {
        goto cleanup;
    }
    ret = SYSTEM_ERROR_UNKNOWN;

    // Allocate ostream
    stream = pb_ostream_init(nullptr);
    if (stream == nullptr) {
        ret = SYSTEM_ERROR_NO_MEMORY;
        goto cleanup;
    }

    res = pb_ostream_from_buffer_ex(stream, (pb_byte_t*)req->reply_data, req->reply_size, nullptr);
    if (!res) {
        goto cleanup;
    }

    res = pb_encode(stream, fields, src);
    if (res) {
        ret = SYSTEM_ERROR_NONE;
    }

cleanup:
    if (stream != nullptr) {
        pb_ostream_free(stream, nullptr);
    }
    if (ret != SYSTEM_ERROR_NONE) {
        system_ctrl_free_reply_data(req, nullptr);
    }
    return ret;
}

int decodeRequestMessage(ctrl_request* req, const pb_field_t* fields, void* dst) {
    pb_istream_t* stream = nullptr;
    int ret = SYSTEM_ERROR_UNKNOWN;
    bool res = false;

    if (req->request_size == 0) {
        // Nothing to decode
        ret = SYSTEM_ERROR_BAD_DATA;
        goto cleanup;
    }

    stream = pb_istream_init(nullptr);
    if (stream == nullptr) {
        ret = SYSTEM_ERROR_NO_MEMORY;
        goto cleanup;
    }

    res = pb_istream_from_buffer_ex(stream, (const pb_byte_t*)req->request_data, req->request_size, nullptr);
    if (!res) {
        goto cleanup;
    }

    res = pb_decode_noinit(stream, fields, dst);
    if (res) {
        ret = SYSTEM_ERROR_NONE;
    } else {
        ret = SYSTEM_ERROR_BAD_DATA;
    }

cleanup:
    if (stream != nullptr) {
        pb_istream_free(stream, nullptr);
    }
    return ret;
}

// Helper classes for working with nanopb's string fields
struct EncodedString {
    const char* data;
    size_t size;

    explicit EncodedString(pb_callback_t* cb, const char* data = nullptr, size_t size = 0) :
            data(data),
            size(size) {
        cb->arg = this;
        cb->funcs.encode = [](pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
            const auto str = (const EncodedString*)*arg;
            if (str->data && str->size > 0 && (!pb_encode_tag_for_field(strm, field) ||
                    !pb_encode_string(strm, (const uint8_t*)str->data, str->size))) {
                return false;
            }
            return true;
        };
    }
};

struct DecodedString {
    const char* data;
    size_t size;

    explicit DecodedString(pb_callback_t* cb) :
            data(nullptr),
            size(0) {
        cb->arg = this;
        cb->funcs.decode = [](pb_istream_t* strm, const pb_field_t* field, void** arg) {
            const size_t n = strm->bytes_left;
            if (n > 0) {
                const auto str = (DecodedString*)*arg;
                str->data = (const char*)strm->state;
                str->size = n;
            }
            return pb_read(strm, nullptr, n);
        };
    }
};

// Class storing a null-terminated string
struct DecodedCString {
    const char* data;
    size_t size;

    explicit DecodedCString(pb_callback_t* cb) :
            data(nullptr),
            size(0) {
        cb->arg = this;
        cb->funcs.decode = [](pb_istream_t* strm, const pb_field_t* field, void** arg) {
            const size_t n = strm->bytes_left;
            if (n > 0) {
                const auto buf = (char*)malloc(n + 1);
                if (!buf) {
                    return false;
                }
                memcpy(buf, strm->state, n);
                buf[n] = '\0';
                const auto str = (DecodedCString*)*arg;
                str->data = buf;
                str->size = n;
            }
            return pb_read(strm, nullptr, n);
        };
    }

    ~DecodedCString() {
        free((char*)data);
    }

    // This class is non-copyable
    DecodedCString(const DecodedCString&) = delete;
    DecodedCString& operator=(const DecodedCString&) = delete;
};

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
    case CTRL_REQUEST_WIFI_GET_ANTENNA: {
        particle_ctrl_WiFiAntennaConfiguration conf = {};
        WLanSelectAntenna_TypeDef ant = wlan_get_antenna(nullptr);
        // Re-map
        switch (ant) {
            case ANT_INTERNAL:
                conf.antenna = particle_ctrl_WiFiAntenna_INTERNAL;
            break;
            case ANT_EXTERNAL:
                conf.antenna = particle_ctrl_WiFiAntenna_EXTERNAL;
            break;
            case ANT_AUTO:
                conf.antenna = particle_ctrl_WiFiAntenna_AUTO;
            break;
        }
        setResult(req, encodeReplyMessage(req, particle_ctrl_WiFiAntennaConfiguration_fields, &conf));
        break;
    }
    case CTRL_REQUEST_WIFI_SET_ANTENNA: {
        particle_ctrl_WiFiAntennaConfiguration conf = {};
        int r = decodeRequestMessage(req, particle_ctrl_WiFiAntennaConfiguration_fields, &conf);
        WLanSelectAntenna_TypeDef ant = ANT_NONE;
        if (r == SYSTEM_ERROR_NONE) {
            // Re-map
            switch (conf.antenna) {
                case particle_ctrl_WiFiAntenna_INTERNAL:
                    ant = ANT_INTERNAL;
                break;
                case particle_ctrl_WiFiAntenna_EXTERNAL:
                    ant = ANT_EXTERNAL;
                break;
                case particle_ctrl_WiFiAntenna_AUTO:
                    ant = ANT_AUTO;
                break;
            }
            if (ant != ANT_NONE) {
                r = wlan_select_antenna(ant) == 0 ? SYSTEM_ERROR_NONE : SYSTEM_ERROR_UNKNOWN;
            } else {
                r = SYSTEM_ERROR_INVALID_ARGUMENT;
            }
        }
        setResult(req, r);
        break;
    }
#endif // Wiring_WiFi
    case CTRL_REQUEST_SET_CLAIM_CODE: {
        particle_ctrl_SetClaimCodeRequest pbReq = {};
        int ret = decodeRequestMessage(req, particle_ctrl_SetClaimCodeRequest_fields, &pbReq);
        if (ret == 0) {
            ret = HAL_Set_Claim_Code(pbReq.code);
        }
        setResult(req, ret);
        break;
    }
    case CTRL_REQUEST_IS_CLAIMED: {
        const int ret = (HAL_IsDeviceClaimed(nullptr) ? SYSTEM_ERROR_NONE : SYSTEM_ERROR_NOT_FOUND);
        setResult(req, ret);
        break;
    }
    case CTRL_REQUEST_SET_SECURITY_KEY: {
        particle_ctrl_SetSecurityKeyRequest pbReq = {};
        DecodedString pbKey(&pbReq.data);
        int ret = decodeRequestMessage(req, particle_ctrl_SetSecurityKeyRequest_fields, &pbReq);
        if (ret == 0) {
            ret = store_security_key_data((security_key_type)pbReq.type, pbKey.data, pbKey.size);
        }
        setResult(req, ret);
        break;
    }
    case CTRL_REQUEST_GET_SECURITY_KEY: {
        particle_ctrl_GetSecurityKeyRequest pbReq = {};
        int ret = decodeRequestMessage(req, particle_ctrl_GetSecurityKeyRequest_fields, &pbReq);
        if (ret == 0) {
            particle_ctrl_GetSecurityKeyReply pbRep = {};
            EncodedString pbKey(&pbRep.data);
            ret = lock_security_key_data((security_key_type)pbReq.type, &pbKey.data, &pbKey.size);
            if (ret == 0) {
                ret = encodeReplyMessage(req, particle_ctrl_GetSecurityKeyReply_fields, &pbRep);
                unlock_security_key_data((security_key_type)pbReq.type);
            }
        }
        setResult(req, ret);
        break;
    }
    case CTRL_REQUEST_SET_SERVER_ADDRESS: {
        particle_ctrl_SetServerAddressRequest pbReq = {};
        int ret = decodeRequestMessage(req, particle_ctrl_SetServerAddressRequest_fields, &pbReq);
        if (ret == 0) {
            ServerAddress addr = {};
            // Check if the address string contains an IP address
            // TODO: Move IP address parsing/encoding to separate functions
            unsigned n1 = 0, n2 = 0, n3 = 0, n4 = 0;
            if (sscanf(pbReq.address, "%u.%u.%u.%u", &n1, &n2, &n3, &n4) == 4) {
                addr.addr_type = IP_ADDRESS;
                addr.ip = ((n1 & 0xff) << 24) | ((n2 & 0xff) << 16) | ((n3 & 0xff) << 8) | (n4 & 0xff);
            } else {
                const size_t n = strlen(pbReq.address);
                if (n < sizeof(ServerAddress::domain)) {
                    addr.addr_type = DOMAIN_NAME;
                    addr.length = n;
                    memcpy(addr.domain, pbReq.address, n);
                } else {
                    ret = SYSTEM_ERROR_TOO_LARGE;
                }
            }
            if (ret == 0) {
                addr.port = pbReq.port;
                ret = store_server_address((server_protocol_type)pbReq.protocol, &addr);
            }
        }
        setResult(req, ret);
        break;
    }
    case CTRL_REQUEST_GET_SERVER_ADDRESS: {
        particle_ctrl_GetServerAddressRequest pbReq = {};
        int ret = decodeRequestMessage(req, particle_ctrl_GetServerAddressRequest_fields, &pbReq);
        if (ret == 0) {
            ServerAddress addr = {};
            ret = load_server_address((server_protocol_type)pbReq.protocol, &addr);
            if (ret == 0) {
                if (addr.addr_type == IP_ADDRESS) {
                    const unsigned n1 = (addr.ip >> 24) & 0xff,
                            n2 = (addr.ip >> 16) & 0xff,
                            n3 = (addr.ip >> 8) & 0xff,
                            n4 = addr.ip & 0xff;
                    const int n = snprintf(addr.domain, sizeof(addr.domain), "%u.%u.%u.%u", n1, n2, n3, n4);
                    if (n > 0 && (size_t)n < sizeof(addr.domain)) {
                        addr.length = n;
                    } else {
                        ret = SYSTEM_ERROR_TOO_LARGE;
                    }
                }
                if (ret == 0) {
                    particle_ctrl_GetServerAddressReply pbRep = {};
                    EncodedString pbAddr(&pbRep.address, addr.domain, addr.length);
                    pbRep.port = addr.port;
                    ret = encodeReplyMessage(req, particle_ctrl_GetServerAddressReply_fields, &pbRep);
                }
            }
        }
        setResult(req, ret);
        break;
    }
    case CTRL_REQUEST_SET_SERVER_PROTOCOL: {
        particle_ctrl_SetServerProtocolRequest pbReq = {};
        int ret = decodeRequestMessage(req, particle_ctrl_SetServerProtocolRequest_fields, &pbReq);
        if (ret == 0) {
            bool udpEnabled = false;
            if (pbReq.protocol == particle_ctrl_ServerProtocolType_TCP_PROTOCOL ||
                    (udpEnabled = (pbReq.protocol == particle_ctrl_ServerProtocolType_UDP_PROTOCOL))) {
                ret = HAL_Feature_Set(FEATURE_CLOUD_UDP, udpEnabled);
            } else {
                ret = SYSTEM_ERROR_NOT_SUPPORTED;
            }
        }
        setResult(req, ret);
        break;
    }
    case CTRL_REQUEST_GET_SERVER_PROTOCOL: {
        particle_ctrl_GetServerProtocolReply pbRep = {};
        if (HAL_Feature_Get(FEATURE_CLOUD_UDP)) {
            pbRep.protocol = particle_ctrl_ServerProtocolType_UDP_PROTOCOL;
        } else {
            pbRep.protocol = particle_ctrl_ServerProtocolType_TCP_PROTOCOL;
        }
        const int ret = encodeReplyMessage(req, particle_ctrl_GetServerProtocolReply_fields, &pbRep);
        setResult(req, ret);
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

#else // !SYSTEM_CONTROL_ENABLED

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
