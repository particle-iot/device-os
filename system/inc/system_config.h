#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "hal_platform.h"

#if HAL_PLATFORM_LOG_FILE

/**
 * Flags for `system_enable_log_file` and `system_disable_log_file`.
 */
typedef enum {
    /**
     * If set, enables or disables logging for the duration of the current session, i.e. until
     * the device is reset.
     *
     * By default, when the flag is not set, the logging configuration is stored persistently and
     * applied automatically when the system is starting up. This allows capturing the early boot
     * messages as well as the messages logged while the device is in safe mode, which otherwise
     * are not visible to the application.
     */
    SYSTEM_LOG_FILE_UNTIL_RESET = 0x01
} system_log_file_flag;

/**
 * Log file options.
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
} system_log_file_options;

/**
 * Callback for `system_read_log_file`.
 *
 * @param data Chunk of log data.
 * @param size Size of the chunk.
 * @param arg User argument.
 * @return 0 on success, otherwise an error code defined by `system_error_t`.
 **/
typedef int (*system_read_log_file_callback)(const char* data, size_t size, void* arg);

#endif // HAL_PLATFORM_LOG_FILE

#ifdef __cplusplus
extern "C" {
#endif

#if HAL_PLATFORM_LOG_FILE

/**
 * Enable logging to a file.
 *
 * @param flags Flags defined by `system_log_file_flag`.
 * @param opts Log file options. If `NULL`, the defaults are used (see `system_log_file_options`).
 * @return 0 on success, otherwise an error code defined by `system_error_t`.
 */
int system_enable_log_file(int flags, const system_log_file_options* opts);

/**
 * Disable logging to a file.
 *
 * @param flags Flags defined by `system_log_file_flag`.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void system_disable_log_file(int flags, void* reserved);

/**
 * Print the contents of the log file.
 *
 * @param size Maximum size of the log data to print. If 0, the entire log is printed.
 * @param level Logging level, as defined by the `LogLevel` enum, at which the contents of the log
 *        are printed.
 * @param category Logging category with which the contents of the log are printed. If `NULL`,
 *        an empty category is used.
 * @return On success, the number of bytes printed, otherwise an error code defined by
 *         `system_error_t`.
 */
int system_print_log_file(size_t size, int level, const char* category, void* reserved);

/**
 * Read the contents of the log file.
 *
 * @param size Maximum size of the log data to read. If 0, the entire log is read.
 * @param callback Callback to invoke for each chunk of the log data.
 * @param arg User argument to pass to the callback.
 * @return On success, the number of bytes read, otherwise an error code defined by
 *         `system_error_t`.
 */
int system_read_log_file(size_t size, system_read_log_file_callback callback, void* arg, void* reserved);

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
