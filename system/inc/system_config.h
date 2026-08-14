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
     * in the log.
     */
    const char* category;
    /**
     * Maximum size of the log in bytes.
     *
     * Up to twice this amount of filesystem storage may be used internally. If 0, a default size
     * is used.
     */
    size_t max_size;
    /**
     * Minimum logging level as defined by the `LogLevel` enum.
     *
     * If 0, all messages will be logged.
     */
    int level;
} system_log_file_config;

/**
 * Options for `system_print_log_file()`.
 */
typedef struct {
    /**
     * Size of this structure.
     */
    size_t size;
    /**
     * Logging category with which to print the contents of the log.
     *
     * If `NULL`, `app` is used.
     */
    const char* category;
    /**
     * Logging level, as defined by the `LogLevel` enum, at which to print the contents of the log.
     *
     * If 0, the contents are printed at the `LOG_LEVEL_INFO` level.
     */
    int level;
} system_log_file_print_options;

#endif // HAL_PLATFORM_LOG_FILE

#ifdef __cplusplus
extern "C" {
#endif

#if HAL_PLATFORM_LOG_FILE

/**
 * Enable the log file.
 *
 * The log file stores the messages logged by the system and the application in the filesystem so
 * that they can be retrieved later, in particular after the device reboots.
 *
 * The log starts capturing the messages immediately. Calling this function again reconfigures the
 * log without losing its contents. The configuration is also stored in the backup RAM so that the
 * log is enabled automatically when the device boots next time.
 *
 * This function accesses the filesystem and must not be called from an ISR.
 *
 * @param config Log file configuration. If `NULL`, the default configuration is used.
 */
void system_enable_log_file(const system_log_file_config* config);

/**
 * Disable the log file.
 *
 * The log stops capturing the messages, the buffered data is discarded and the contents of the log
 * are deleted. The log is not enabled automatically when the device boots next time.
 *
 * This function accesses the filesystem and must not be called from an ISR.
 *
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void system_disable_log_file(void* reserved);

/**
 * Write the buffered log data to the file.
 *
 * The messages are buffered in RAM and written to the file periodically, so this function can be
 * used to make sure that nothing is lost before the device is reset. The log keeps capturing the
 * messages.
 *
 * This function accesses the filesystem and must not be called from an ISR.
 *
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void system_flush_log_file(void* reserved);

/**
 * Print the contents of the log file.
 *
 * The buffered data is written to the file first, so that the printed contents are complete. The
 * messages generated while the log is being printed are not stored in the log. The log keeps
 * capturing the messages otherwise.
 *
 * This function accesses the filesystem and must not be called from an ISR.
 *
 * @param opts Options. If `NULL`, the defaults are used (see `system_log_file_print_options`).
 */
void system_print_log_file(const system_log_file_print_options* opts);

/**
 * Delete the contents of the log file.
 *
 * The buffered data is discarded as well. The log keeps capturing the messages.
 *
 * This function accesses the filesystem and must not be called from an ISR.
 *
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void system_clear_log_file(void* reserved);

#endif // HAL_PLATFORM_LOG_FILE

#ifdef __cplusplus
} // extern "C"
#endif
