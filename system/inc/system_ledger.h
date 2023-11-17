/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#include "hal_platform.h"

#if HAL_PLATFORM_LEDGER

#include <stddef.h>
#include <stdint.h>

/**
 * Ledger API version.
 */
#define LEDGER_API_VERSION 1

/**
 * Maximum length of a ledger name.
 */
#define LEDGER_MAX_NAME_LENGTH 32

/**
 * Maximum size of ledger data.
 */
#define LEDGER_MAX_DATA_SIZE 16384

/**
 * Ledger instance.
 */
typedef struct ledger_instance ledger_instance;

/**
 * Stream instance.
 */
typedef struct ledger_stream ledger_stream;

/**
 * Callback invoked when a ledger has been synchronized with the Cloud.
 *
 * @param ledger Ledger instance.
 * @param app_data Application data.
 */
typedef void (*ledger_sync_callback)(ledger_instance* ledger, void* app_data);

/**
 * Callback invoked to destroy the application data associated with a ledger instance.
 *
 * @param app_data Application data.
 */
typedef void (*ledger_destroy_app_data_callback)(void* app_data);

/**
 * Ledger scope.
 */
typedef enum ledger_scope {
    LEDGER_SCOPE_UNKNOWN = 0, ///< Unknown scope.
    LEDGER_SCOPE_DEVICE = 1, ///< Device scope.
    LEDGER_SCOPE_PRODUCT = 2, ///< Product scope.
    LEDGER_SCOPE_OWNER = 3 ///< Owner scope.
} ledger_scope;

/**
 * Sync direction.
 */
typedef enum ledger_sync_direction {
    LEDGER_SYNC_DIRECTION_UNKNOWN = 0, ///< Unknown direction.
    LEDGER_SYNC_DIRECTION_DEVICE_TO_CLOUD = 1, ///< Device to cloud.
    LEDGER_SYNC_DIRECTION_CLOUD_TO_DEVICE = 2 ///< Cloud to device.
} ledger_sync_direction;

/**
 * Ledger info flags.
 */
typedef enum ledger_info_flag {
    LEDGER_INFO_SYNC_PENDING = 0x01 ///< Ledger has changes that have not yet been synchronized with the Cloud.
} ledger_info_flag;

/**
 * Stream mode flags.
 */
typedef enum ledger_stream_mode {
    LEDGER_STREAM_MODE_READ = 0x01, ///< Open for reading.
    LEDGER_STREAM_MODE_WRITE = 0x02 ///< Open for writing.
} ledger_stream_mode;

/**
 * Stream close flags.
 */
typedef enum ledger_stream_close_flag {
    LEDGER_STREAM_CLOSE_DISCARD = 0x01 ///< Discard any written data.
} ledger_stream_close_flag;

/**
 * Ledger callbacks.
 */
typedef struct ledger_callbacks {
    int version; ///< API version. Must be set to `LEDGER_API_VERSION`.
    /**
     * Callback invoked when the ledger has been synchronized with the Cloud.
     */
    ledger_sync_callback sync;
} ledger_callbacks;

/**
 * Ledger info.
 */
typedef struct ledger_info {
    int version; ///< API version. Must be set to `LEDGER_API_VERSION`.
    const char* name; ///< Ledger name.
    /**
     * Time the ledger was last updated, in milliseconds since the Unix epoch.
     *
     * If 0, the time is unknown.
     */
    int64_t last_updated;
    /**
     * Time the ledger was last synchronized with the Cloud, in milliseconds since the Unix epoch.
     *
     * If 0, the ledger has never been synchronized.
     */
    int64_t last_synced;
    size_t data_size; ///< Size of the ledger data in bytes.
    int scope; ///< Ledger scope as defined by the `ledger_scope` enum.
    int sync_direction; ///< Synchronization direction as defined by the `ledger_sync_direction` enum.
    int flags; ///< Flags defined by the `ledger_info_flag` enum.
} ledger_info;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get a ledger instance.
 *
 * @param ledger[out] Ledger instance.
 * @param name Ledger name.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_get_instance(ledger_instance** ledger, const char* name, void* reserved);

/**
 * Increment a ledger's reference count.
 *
 * @param ledger Ledger instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void ledger_add_ref(ledger_instance* ledger, void* reserved);

/**
 * Decrement a ledger's reference count.
 *
 * The ledger instance is destroyed when its reference count reaches 0.
 *
 * @param ledger Ledger instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void ledger_release(ledger_instance* ledger, void* reserved);

/**
 * Lock a ledger instance.
 *
 * The instance can be locked recursively by the same thread.
 *
 * @param ledger Ledger instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void ledger_lock(ledger_instance* ledger, void* reserved);

/**
 * Unlock a ledger instance.
 *
 * @param ledger Ledger instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void ledger_unlock(ledger_instance* ledger, void* reserved);

/**
 * Set the ledger callbacks.
 *
 * All callbacks are invoked in the system thread.
 *
 * @param ledger Ledger instance.
 * @param callbacks Ledger callbacks. Can be set to `NULL` to clear all currently registered callbacks.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void ledger_set_callbacks(ledger_instance* ledger, const ledger_callbacks* callbacks, void* reserved);

/**
 * Attach application-specific data to a ledger instance.
 *
 * If a destructor callback is provided, it will be invoked when the ledger instance is destroyed.
 * The calling code is responsible for destroying the old application data if it was already set for
 * this ledger instance.
 *
 * @param ledger Ledger instance.
 * @param app_data Application data.
 * @param destroy Destructor callback. Can be `NULL`.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void ledger_set_app_data(ledger_instance* ledger, void* app_data, ledger_destroy_app_data_callback destroy,
        void* reserved);

/**
 * Get application-specific data associated with a ledger instance.
 *
 * @param ledger Ledger instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return Application data.
 */
void* ledger_get_app_data(ledger_instance* ledger, void* reserved);

/**
 * Get ledger info.
 *
 * @param ledger Ledger instance.
 * @param[out] info Ledger info to be populated.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_get_info(ledger_instance* ledger, ledger_info* info, void* reserved);

/**
 * Open a ledger for reading or writing.
 *
 * @param[out] stream Stream instance.
 * @param ledger Ledger instance.
 * @param mode Flags defined by the `ledger_stream_mode` enum.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_open(ledger_stream** stream, ledger_instance* ledger, int mode, void* reserved);

/**
 * Close a stream.
 *
 * The stream instance is destroyed even if an error occurs while closing the stream.
 *
 * @param stream Stream instance.
 * @param flags Flags defined by the `ledger_stream_close_flag` enum.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_close(ledger_stream* stream, int flags, void* reserved);

/**
 * Read from a stream.
 *
 * @param stream Stream instance.
 * @param[out] data Output buffer.
 * @param size Maximum number of bytes to read.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return Number of bytes read or an error code defined by the `system_error_t` enum.
 */
int ledger_read(ledger_stream* stream, char* data, size_t size, void* reserved);

/**
 * Write to a stream.
 *
 * @param stream Stream instance.
 * @param data Input buffer.
 * @param size Number of bytes to write.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return Number of bytes written or an error code defined by the `system_error_t` enum.
 */
int ledger_write(ledger_stream* stream, const char* data, size_t size, void* reserved);

/**
 * Get the names of all local ledgers.
 *
 * @param[out] names Array of ledger names. The calling code is responsible for freeing the allocated
 *        array as well as its individual elements via `free()`.
 * @param[out] count Number of elements in the array.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_get_names(char*** names, size_t* count, void* reserved);

/**
 * Remove any local data associated with a ledger.
 *
 * The device must not be connected to the Cloud. The operation will fail if the ledger with the
 * given name is in use.
 *
 * @note The data is not guaranteed to be removed in an irrecoverable way.
 *
 * @param name Ledger name.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_purge(const char* name, void* reserved);

/**
 * Remove any local data associated with existing ledgers.
 *
 * The device must not be connected to the Cloud. The operation will fail if any of the existing
 * ledgers is in use.
 *
 * @note The data is not guaranteed to be removed in an irrecoverable way.
 *
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_purge_all(void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HAL_PLATFORM_LEDGER
