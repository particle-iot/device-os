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
     * Size of the buffer for log data.
     *
     * TODO: Describe what the parameter actually does.
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
     * Logging category.
     *
     * TODO: More docs.
     */
    const char* category;
    /**
     * Maximum size of the log data to print.
     *
     * TODO: More docs.
     */
    size_t max_size;
    /**
     * Logging level.
     *
     * TODO: More docs.
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
 * The configuration is stored persistently so that the log is enabled automatically when
 * the device boots next time.
 *
 * @param config Log file configuration. If `NULL`, the default configuration is used.
 */
int system_enable_log_file(const system_log_file_config* config);

/**
 * Disable logging to a file.
 *
 * TODO: Describe what the function actually does.
 *
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void system_disable_log_file(void* reserved);

/**
 * Print the contents of the log file.
 *
 * @param size Maximum size of the log data to print. If 0, prints the entire log.
 * @param level Level at which to print the contents of the log.
 * @param category Category with which to print the contents of the log.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
int system_print_log_file(const system_print_log_file_options* opts);

/**
 * Delete all log files.
 *
 * The log keeps capturing the messages.
 *
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
int system_clear_log_file(void* reserved);

#endif // HAL_PLATFORM_LOG_FILE

#ifdef __cplusplus
} // extern "C"
#endif
