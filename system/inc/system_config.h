#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "hal_platform.h"

#if HAL_PLATFORM_LOG_FILE

/**
 * Log file configuration.
 */
typedef struct {
    /**
     * Size of this structure.
     */
    size_t size;
    /**
     * Logging category.
     *
     * If not `NULL`, only the messages logged for a category starting with this string will be stored
     * in the log file.
     */
    const char* category;
    /**
     * Maximum size of the log data in bytes.
     *
     * Specifies how much of the most recent log data can be stored in the filesystem. Up to twice this
     * amount of filesystem storage may be used internally.
     *
     * If 0, a default size is used.
     */
    size_t max_size;
    /**
     * Size of the buffer for log data in bytes.
     *
     * The messages are stored in a buffer in RAM and written to the file periodically. A larger buffer
     * makes it less likely that the messages are dropped when they are logged faster than they can be
     * written to the file.
     *
     * If 0, a default size is used.
     */
    size_t buffer_size;
    /**
     * Minimum logging level as defined by the `LogLevel` enum.
     *
     * If 0, messages at any level will be logged.
     */
    int level;
} system_log_file_config;

/**
 * Options for `system_print_log_file`.
 */
typedef struct {
    /**
     * Size of this structure.
     */
    size_t size;
    /**
     * Logging category with which the contents of the log are printed.
     *
     * If `NULL`, an empty category is used.
     */
    const char* category;
    /**
     * Maximum size of the log data to print in bytes.
     *
     * If the log contains more data than that, only the most recent `max_size` bytes are printed.
     *
     * If 0, the entire log is printed.
     */
    size_t max_size;
    /**
     * Logging level, as defined by the `LogLevel` enum, at which the contents of the log are printed.
     *
     * If 0, `LOG_LEVEL_INFO` is used.
     */
    int level;
} system_print_log_file_options;

#endif // HAL_PLATFORM_LOG_FILE

#ifdef __cplusplus
extern "C" {
#endif

#if HAL_PLATFORM_LOG_FILE

/**
 * Enable logging to a file.
 *
 * The configuration is stored persistently so that the log is enabled automatically when the device
 * boots next time.
 *
 * @param config Log file configuration. If `NULL`, the default configuration is used.
 * @return 0 on success, otherwise an error code defined by `system_error_t`.
 */
int system_enable_log_file(const system_log_file_config* config);

/**
 * Disable logging to a file.
 *
 * The log will not be enabled again when the device boots next time.
 *
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void system_disable_log_file(void* reserved);

/**
 * Print the contents of the log file.
 *
 * @param opts Options. If `NULL`, the default options are used.
 * @return 0 on success, otherwise an error code defined by `system_error_t`.
 */
int system_print_log_file(const system_print_log_file_options* opts);

/**
 * Delete all log files.
 *
 * The log keeps capturing the messages.
 *
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by `system_error_t`.
 */
int system_clear_log_file(void* reserved);

#endif // HAL_PLATFORM_LOG_FILE

#ifdef __cplusplus
} // extern "C"
#endif
