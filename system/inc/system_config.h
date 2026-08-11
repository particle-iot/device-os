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

#endif // HAL_PLATFORM_BOOT_LOG

#ifdef __cplusplus
extern "C" {
#endif

#if HAL_PLATFORM_BOOT_LOG

/**
 * Enable or disable the boot log.
 *
 * The boot log stores the messages logged while the device is booting so that they can be retrieved
 * by the application once it starts.
 *
 * The configuration is stored in the backup RAM and applied when the device boots next time.
 *
 * @param enabled Whether the boot log is enabled.
 * @param config Boot log configuration. If `NULL`, the default configuration is used.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void system_enable_boot_log(bool enabled, const system_boot_log_config* config, void* reserved);

/**
 * Print the contents of the boot log and delete it.
 *
 * @param level Logging level at which the contents of the boot log are written.
 * @param category Logging category with which the contents of the boot log are written.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void system_flush_boot_log(int level, const char* category, void* reserved);

#endif // HAL_PLATFORM_BOOT_LOG

#ifdef __cplusplus
} // extern "C"
#endif
