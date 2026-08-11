#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "hal_platform.h"

#if HAL_PLATFORM_BOOT_LOG

/**
 * Boot log configuration.
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
} system_boot_log_config;

/**
 * Options for `system_flush_boot_log()`.
 */
typedef struct {
    /**
     * Size of this structure.
     */
    size_t size;
    /**
     * Logging category with which to print the contents of the log.
     *
     * If `NULL`, `system.boot` is used.
     */
    const char* category;
    /**
     * Logging level, as defined by the `LogLevel` enum, at which to print the contents of the log.
     *
     * If 0, the contents are printed at the `LOG_LEVEL_INFO` level.
     */
    int level;
} system_boot_log_flush_options;

#endif // HAL_PLATFORM_BOOT_LOG

#ifdef __cplusplus
extern "C" {
#endif

#if HAL_PLATFORM_BOOT_LOG

/**
 * Enable the boot log.
 *
 * The boot log stores the messages logged while the device is booting so that they can be retrieved
 * by the application once it starts.
 *
 * The configuration is stored in the backup RAM and applied when the device boots next time.
 *
 * @param config Boot log configuration. If `NULL`, the default configuration is used.
 */
void system_enable_boot_log(const system_boot_log_config* config);

/**
 * Disable the boot log.
 *
 * Prevents the device from writing to the boot log when it boots next time.
 *
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void system_disable_boot_log(void* reserved);

/**
 * Print the contents of the boot log and delete it.
 *
 * @param opts Options. If `NULL`, the defaults are used (see `system_boot_log_flush_options`).
 */
void system_flush_boot_log(const system_boot_log_flush_options* opts);

#endif // HAL_PLATFORM_BOOT_LOG

#ifdef __cplusplus
} // extern "C"
#endif
