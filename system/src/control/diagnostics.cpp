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

#include "diagnostics.h"

#if SYSTEM_CONTROL_ENABLED

#include "system_logging.h"
#include "common.h"

#include "c_string.h"
#include "scope_guard.h"
#include "check.h"

#include "spark_wiring_vector.h"

#include "diagnostics.pb.h"

#define PB(_name) particle_ctrl_diagnostics_##_name

namespace particle {

namespace control {

namespace diagnostics {

namespace {

using namespace common;

// Maximum length of a handler ID
const size_t MAX_HANDLER_ID_SIZE = 32;

// Maximum number of category filters per handler
const size_t MAX_FILTER_COUNT = 64;

} // unnamed

int addLogHandler(ctrl_request* req) {
    struct Filters {
        Vector<log_filter> filters; // Category filters
        Vector<CString> strings; // Category name strings
    };
    PB(AddLogHandlerRequest) pbReq = {};
    const DecodedCString idStr(&pbReq.id);
    Filters f;
    pbReq.filters.arg = &f;
    pbReq.filters.funcs.decode = [](pb_istream_t* strm, const pb_field_t* field, void** arg) {
        const auto f = (Filters*)*arg;
        if ((size_t)f->filters.size() >= MAX_FILTER_COUNT) {
            return false;
        }
        PB(LogFilter) pbFilt = {};
        const DecodedString ctgStr(&pbFilt.category);
        if (!pb_decode_noinit(strm, PB(LogFilter_fields), &pbFilt)) {
            return false;
        }
        // Store the category name as a null-terminated string
        CString ctg(ctgStr.data, ctgStr.size);
        if (!ctg) {
            return false;
        }
        log_filter filt;
        filt.category = ctg; // Category name
        filt.level = pbFilt.level; // Logging level
        if (!f->filters.append(std::move(filt)) || !f->strings.append(std::move(ctg))) {
            return false;
        }
        return true;
    };
    CHECK(decodeRequestMessage(req, PB(AddLogHandlerRequest_fields), &pbReq));
    if (idStr.size == 0 || idStr.size > MAX_HANDLER_ID_SIZE) {
        return SYSTEM_ERROR_OUT_OF_RANGE;
    }
    log_config_add_handler_command cmd = {};
    cmd.version = LOG_CONFIG_API_VERSION; // API version
    cmd.id = idStr.data; // Handler ID
    cmd.type = pbReq.type; // Handler type
    cmd.level = pbReq.level; // Default logging level
    // Handler-specific settings
    log_config_serial_params params = {};
    if (pbReq.which_settings == PB(AddLogHandlerRequest_serial_tag)) {
        params.index = pbReq.settings.serial.index;
        params.baud_rate = pbReq.settings.serial.baud_rate;
        cmd.params = &params;
    }
    // Category filters
    cmd.filters = f.filters.data();
    cmd.filter_count = f.filters.size();
    CHECK(log_config(LOG_CONFIG_ADD_HANDLER, &cmd, nullptr));
    return 0;
}

int removeLogHandler(ctrl_request* req) {
    PB(RemoveLogHandlerRequest) pbReq = {};
    const DecodedCString idStr(&pbReq.id);
    CHECK(decodeRequestMessage(req, PB(RemoveLogHandlerRequest_fields), &pbReq));
    log_config_remove_handler_command cmd = {};
    cmd.version = LOG_CONFIG_API_VERSION; // API version
    cmd.id = idStr.data; // Handler ID
    CHECK(log_config(LOG_CONFIG_REMOVE_HANDLER, &cmd, nullptr));
    return 0;
}

int getLogHandlers(ctrl_request* req) {
    log_config_get_handlers_command cmd = {};
    cmd.version = LOG_CONFIG_API_VERSION; // API version
    log_config_get_handlers_result result = {};
    CHECK(log_config(LOG_CONFIG_GET_HANDLERS, &cmd, &result));
    // The handler list is allocated dynamically and needs to be freed
    SCOPE_GUARD({
        for (size_t i = 0; i < result.handler_count; ++i) {
            free(result.handlers[i].id);
        }
        free(result.handlers);
    });
    // Encode a reply message
    PB(GetLogHandlersReply) pbRep = {};
    pbRep.handlers.arg = &result;
    pbRep.handlers.funcs.encode = [](pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
        const auto result = (const log_config_get_handlers_result*)*arg;
        for (int i = 0; i < result->handler_count; ++i) {
            PB(GetLogHandlersReply_Handler) pbHandler = {};
            const auto id = result->handlers[i].id;
            const EncodedString idStr(&pbHandler.id, id, strlen(id));
            if (!pb_encode_tag_for_field(strm, field) ||
                    !pb_encode_submessage(strm, PB(GetLogHandlersReply_Handler_fields), &pbHandler)) {
                return false;
            }
        }
        return true;
    };
    CHECK(encodeReplyMessage(req, PB(GetLogHandlersReply_fields), &pbRep));
    return 0;
}

} // particle::control::diagnostics

} // particle::control

} // particle

#endif // SYSTEM_CONTROL_ENABLED
