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

#include "log_config.h"

#if SYSTEM_CONTROL_ENABLED

#include "system_logging.h"
#include "common.h"

#include "c_string.h"
#include "check.h"
#include "debug.h"

#include "spark_wiring_vector.h"

#include "logging.pb.h"

#define PB(_name) particle_ctrl_logging_##_name

namespace particle {

namespace control {

namespace logging {

namespace {

using namespace common;

// Maximum length of a handler ID
const size_t MAX_HANDLER_ID_SIZE = 32;

// Maximum number of category filters per handler
const size_t MAX_FILTER_COUNT = 64;

int addLogHandlerImpl(ctrl_request* req, log_command_result** result) {
    struct Filter: log_filter_list_item {
        CString categoryName;
    };
    PB(AddLogHandlerRequest) pbReq = {};
    const DecodedCString idStr(&pbReq.id);
    Vector<Filter> filters;
    pbReq.filters.arg = &filters;
    pbReq.filters.funcs.decode = [](pb_istream_t* strm, const pb_field_t* field, void** arg) {
        const auto filters = (Vector<Filter>*)*arg;
        if ((size_t)filters->size() >= MAX_FILTER_COUNT) {
            return false;
        }
        PB(LogFilter) pbFilter = {};
        const DecodedString ctgStr(&pbFilter.category);
        if (!pb_decode_noinit(strm, PB(LogFilter_fields), &pbFilter)) {
            return false;
        }
        if (ctgStr.size == 0) {
            return false; // Empty category names are not allowed
        }
        Filter f = {};
        // Store the category name as a null-terminated string
        f.categoryName = CString(ctgStr.data, ctgStr.size);
        if (!f.categoryName) {
            return false; // Memory allocation error
        }
        f.category = f.categoryName; // Category name
        f.level = pbFilter.level; // Logging level
        if (!filters->append(std::move(f))) {
            return false;
        }
        if (filters->size() > 1) {
            filters->at(filters->size() - 2).next = &filters->last();
        }
        return true;
    };
    CHECK(decodeRequestMessage(req, PB(AddLogHandlerRequest_fields), &pbReq));
    if (idStr.size == 0 || idStr.size > MAX_HANDLER_ID_SIZE) {
        return SYSTEM_ERROR_OUT_OF_RANGE;
    }
    log_add_handler_command cmd = {};
    cmd.command.type = LOG_ADD_HANDLER_COMMAND; // Command type
    // Handler parameters
    log_handler handler = {};
    handler.id = idStr.data; // Handler ID
    handler.type = pbReq.handler_type; // Handler type
    handler.level = pbReq.level; // Default logging level
    // Category filters
    handler.filters = filters.data();
    handler.filter_count = filters.size();
    cmd.handler = &handler;
    // Stream parameters
    log_serial_stream serial = {};
    if (pbReq.which_stream_params == PB(AddLogHandlerRequest_serial_tag)) {
        serial.stream.type = pbReq.stream_type; // Stream type
        serial.index = pbReq.stream_params.serial.index; // Interface index
        serial.baud_rate = pbReq.stream_params.serial.baud_rate; // Baud rate
    }
    cmd.stream = (log_stream*)&serial;
    CHECK(log_process_command((log_command*)&cmd, result));
    return 0;
}

int removeLogHandlerImpl(ctrl_request* req, log_command_result** result) {
    PB(RemoveLogHandlerRequest) pbReq = {};
    const DecodedCString idStr(&pbReq.id);
    CHECK(decodeRequestMessage(req, PB(RemoveLogHandlerRequest_fields), &pbReq));
    log_remove_handler_command cmd = {};
    cmd.command.type = LOG_REMOVE_HANDLER_COMMAND; // Command type
    cmd.id = idStr.data; // Handler ID
    CHECK(log_process_command((log_command*)&cmd, result));
    return 0;
}

int getLogHandlersImpl(ctrl_request* req, log_command_result** result) {
    log_command cmd = {};
    cmd.type = LOG_GET_HANDLERS_COMMAND;
    CHECK(log_process_command(&cmd, result));
    SPARK_ASSERT(*result);
    // Encode a reply message
    PB(GetLogHandlersReply) pbRep = {};
    pbRep.handlers.arg = const_cast<log_command_result*>(*result);
    pbRep.handlers.funcs.encode = [](pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
        const auto result = (const log_get_handlers_result*)*arg;
        for (auto handler = result->handlers; handler; handler = handler->next) {
            PB(GetLogHandlersReply_Handler) pbHandler = {};
            SPARK_ASSERT(handler->id);
            const EncodedString idStr(&pbHandler.id, handler->id, strlen(handler->id));
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

void setResult(ctrl_request* req, int error, log_command_result* result) {
    if (result) {
        if (result->completion_fn) {
            // Wait until the request finishes and then invoke the completion callback and free the result data
            system_ctrl_set_result(req, error, [](int error, void* data) {
                const auto result = (log_command_result*)data;
                result->completion_fn(error, result);
                if (result->deleter_fn) {
                    result->deleter_fn(result);
                }
            }, result, nullptr);
        } else {
            // No completion callback: free the result data and finish the request
            if (result->deleter_fn) {
                result->deleter_fn(result);
            }
            system_ctrl_set_result(req, error, nullptr, nullptr, nullptr);
        }
    } else {
        // No result data: finish the request
        system_ctrl_set_result(req, error, nullptr, nullptr, nullptr);
    }
}

} // unnamed

void addLogHandler(ctrl_request* req) {
    log_command_result* result = nullptr;
    const int error = addLogHandlerImpl(req, &result);
    setResult(req, error, result);
}

void removeLogHandler(ctrl_request* req) {
    log_command_result* result = nullptr;
    const int error = removeLogHandlerImpl(req, &result);
    setResult(req, error, result);
}

void getLogHandlers(ctrl_request* req) {
    log_command_result* result = nullptr;
    const int error = getLogHandlersImpl(req, &result);
    setResult(req, error, result);
}

} // particle::control::logging

} // particle::control

} // particle

#endif // SYSTEM_CONTROL_ENABLED
