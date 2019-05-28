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

#include <stdint.h>

/**
 * API version number.
 */
#define LOG_SYSTEM_API_VERSION 1

typedef struct log_command log_command;
typedef struct log_command_result log_command_result;
typedef struct log_handler_list_item log_handler_list_item;
typedef struct log_filter_list_item log_filter_list_item;

/**
 * Command handler callback.
 *
 * @param cmd Command data.
 * @param result[out] Result data.
 * @param user_data User data.
 * @return 0 on success, or a negative result code in case of an error.
 *
 * @see `log_set_command_handler()`
 */
typedef int(*log_command_handler_fn)(const log_command* cmd, log_command_result** result, void* user_data);

/**
 * Command completion callback.
 *
 * @param error Result code.
 * @param result Result data.
 *
 * @see `log_command_result`
 */
typedef void(*log_command_completion_fn)(int error, log_command_result* result);

/**
 * Deleter function for a command result.
 *
 * @param result Result data.
 *
 * @see `log_command_result`
 */
typedef void(*log_command_result_deleter_fn)(log_command_result* result);

/**
 * Command types.
 */
typedef enum log_command_type {
    LOG_INVALID_COMMAND = 0, ///< Invalid command.
    LOG_ADD_HANDLER_COMMAND = 1, ///< Add a log handler.
    LOG_REMOVE_HANDLER_COMMAND = 2, ///< Remove a log handler.
    LOG_GET_HANDLERS_COMMAND = 3 ///< Get the list of active log handlers.
} log_command_type;

/**
 * Log handler types.
 */
typedef enum log_handler_type {
    LOG_INVALID_HANDLER = 0, ///< Invalid handler.
    LOG_DEFAULT_STREAM_HANDLER = 1, ///< `StreamLogHandler`.
    LOG_JSON_STREAM_HANDLER = 2 ///< `JSONStreamLogHandler`.
} log_handler_type;

/**
 * Stream types.
 */
typedef enum log_stream_type {
    LOG_INVALID_STREAM = 0, ///< Invalid stream.
    LOG_USB_SERIAL_STREAM = 1, ///< USB serial (`Serial`, `USBSerial1`, etc.)
    LOG_HW_SERIAL_STREAM = 2 ///< Hardware serial (`Serial1`, `Serial2`, etc.)
} log_stream_type;

/**
 * Common handler parameters.
 */
typedef struct log_handler {
    const char* id; ///< Handler ID.
    const log_filter_list_item* filters; ///< List of category filters.
    uint16_t filter_count; ///< Number of category filters.
    uint8_t type; ///< Handler type (a value defined by the `log_handler_type` enum).
    uint8_t level; ///< Default logging level (a value defined by the `LogLevel` enum).
} __attribute__((aligned(4))) log_handler;
// ^^ makes sure the first field of a derived structure is aligned by at least 4 bytes

/**
 * Common stream parameters.
 */
typedef struct log_stream {
    uint8_t type; ///< Stream type (a value defined by the `log_stream_type` enum).
} __attribute__((aligned(4))) log_stream;

/**
 * Common command data.
 */
typedef struct log_command {
    uint8_t type; ///< Command type (a value defined by the `log_command_type` enum).
} __attribute__((aligned(4))) log_command;

/**
 * Common result data.
 */
typedef struct log_command_result {
    log_command_completion_fn completion_fn; ///< Command completion callback.
    log_command_result_deleter_fn deleter_fn; ///< Result deleter callback.
    uint16_t version; ///< API version number.
} __attribute__((aligned(4))) log_command_result;

/**
 * Serial stream parameters.
 */
typedef struct log_serial_stream {
    log_stream stream; ///< Common stream parameters.
    uint32_t baud_rate; ///< Baud rate.
    uint8_t index; ///< Interface index.
} log_serial_stream;

/**
 * Parameters of the `LOG_ADD_HANDLER_COMMAND` command.
 */
typedef struct log_add_handler_command {
    log_command command; ///< Common command data.
    const log_handler* handler; ///< Handler parameters.
    const log_stream* stream; ///< Stream parameters.
} log_add_handler_command;

/**
 * Parameters of the `LOG_REMOVE_HANDLER_COMMAND` command.
 */
typedef struct log_remove_handler_command {
    log_command command; ///< Common command data.
    const char* id; ///< Handler ID.
} log_remove_handler_command;

/**
 * Result of a `LOG_CONFIG_GET_HANDLERS` command.
 */
typedef struct log_get_handlers_result {
    log_command_result result; ///< Common result data.
    const log_handler_list_item* handlers; ///< List of active handlers.
    uint16_t handler_count; ///< Number of active handlers.
} log_get_handlers_result;

/**
 * An element of a log handler list.
 */
typedef struct log_handler_list_item {
    const log_handler_list_item* next; ///< Next element in the list.
    const char* id; ///< Handler ID.
} log_handler_list_item;

/**
 * An element of a filter list.
 */
typedef struct log_filter_list_item {
    const log_filter_list_item* next; ///< Next element in the list.
    const char* category; ///< Category name.
    uint8_t level; ///< Logging level (a value defined by the `LogLevel` enum).
} log_filter_list_item;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Set the command handler callback.
 *
 * @param handler A callback.
 * @param user_data User data.
 * @param reserved This argument should be set to NULL.
 */
void log_set_command_handler(log_command_handler_fn handler, void* user_data, void* reserved);

/**
 * Process a logging configuration command.
 *
 * This function is used internally by the system module.
 *
 * @param cmd Command data.
 * @param result[out] Result data.
 * @return 0 on success, or a negative result code in case of an error.
 */
int log_process_command(const log_command* cmd, log_command_result** result);

#ifdef __cplusplus
} // extern "C"
#endif
