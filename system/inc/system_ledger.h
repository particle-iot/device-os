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

#include <stddef.h>
#include <stdint.h>

/**
 * Ledger API version.
 */
#define LEDGER_API_VERSION 1

/**
 * Ledger instance.
 */
struct ledger_instance;
typedef struct ledger_instance ledger_instance;

/**
 * Page instance.
 */
struct ledger_page;
typedef struct ledger_page ledger_page;

/**
 * Stream instance.
 */
struct ledger_stream;
typedef struct ledger_stream ledger_stream;

/**
 * Callback invoked when a page has been synchronized with the Cloud or an error occured during
 * the synchronization.
 *
 * @param ledger Ledger instance.
 * @param page_name Page name.
 * @param error 0 if the synchronization succeeded, otherwise an error code defined by the `system_error_t` enum.
 * @param user_arg User argument provided when the callback was registered.
 */
typedef void (*ledger_page_sync_callback)(ledger_instance* ledger, const char* page_name, int error, void* user_arg);

/**
 * Callback invoked when a page has changed in the Cloud.
 *
 * @param ledger Ledger instance.
 * @param page_name Page name.
 * @param flags Flags defined by the `ledger_page_change_flag` enum.
 * @param user_arg User argument provided when the callback was registered.
 */
typedef void (*ledger_page_change_callback)(ledger_instance* ledger, const char* page_name, int flags, void* user_arg);

/**
 * Callback invoked to destroy the application-specific data associated with a ledger or page instance.
 *
 * @param app_data Application data.
 * @param user_arg User argument provided when the callback was registered.
 */
typedef void (*ledger_destroy_app_data_callback)(void* app_data, void* user_arg);

/**
 * Ledger scope.
 */
typedef enum ledger_scope {
    LEDGER_SCOPE_UNKNOWN = 0, ///< Unknown scope.
    LEDGER_SCOPE_DEVICE = 1, ///< Device scope.
    LEDGER_SCOPE_PRODUCT = 2, ///< Product scope.
    LEDGER_SCOPE_ORG = 3 ///< Organization scope.
} ledger_scope;

/**
 * Page info flags.
 */
typedef enum ledger_page_info_flag {
    LEDGER_PAGE_INFO_FLAG_SYNCING = 0x01 ///< Synchronization is in progress for this page.
} ledger_page_info_flag;

/**
 * Page change flags.
 */
typedef enum ledger_page_change_flag {
    LEDGER_PAGE_CHANGE_FLAG_PAGE_REMOVED = 0x01 ///< Page was removed by the remote side.
} ledger_page_change_flag;

/**
 * Synchronization strategy.
 */
typedef enum ledger_sync_strategy {
    LEDGER_SYNC_STRATEGY_DEFAULT = 0, ///< Use the default strategy.
    LEDGER_SYNC_STRATEGY_PREFER_LOCAL_CHANGES = 1, ///< Prefer local changes.
    LEDGER_SYNC_STRATEGY_PREFER_REMOTE_CHANGES = 2 ///< Prefer remote changes.
} ledger_sync_strategy;

/**
 * Stream mode flags.
 */
typedef enum ledger_stream_mode {
    LEDGER_STREAM_MODE_READ = 0x01, ///< Open for reading.
    LEDGER_STREAM_MODE_WRITE = 0x02 ///< Open for writing.
} ledger_stream_mode;

/**
 * Ledger callbacks.
 */
typedef struct ledger_callbacks {
    ledger_page_sync_callback page_sync; ///< Callback invoked when a page has been synchronized with the Cloud.
    ledger_page_change_callback page_change; ///< Callback invoked when a page has changed in the Cloud.
} ledger_callbacks;

/**
 * Synchronization options.
 */
typedef struct ledger_sync_options {
    int strategy; ///< Synchronization strategy as defined by the `ledger_sync_strategy` enum.
} ledger_sync_options;

/**
 * Ledger info.
 */
typedef struct ledger_info {
    const char* name; ///< Ledger name.
    int scope; ///< Ledger scope.
    /**
     * Names of the linked pages.
     *
     * The calling code is responsible for allocating this array as well as for freeing the memory
     * allocated by the system for each of the array elements.
     */
    const char** linked_page_names;
    /**
     * Number of the linked pages.
     *
     * The system returns at most this number of elements in the `linked_page_names` array and sets
     * this field to the actual number of linked pages of this ledger.
     */
    size_t linked_page_count;
} ledger_info;

/**
 * Page info.
 */
typedef struct ledger_page_info {
    const char* name; ///< Page name.
    size_t data_size; ///< Size of the page contents.
    uint64_t last_sync_time; ///< Last time the page was synchronized with the Cloud, in milliseconds since the Unix epoch.
    int flags; ///< Flags defined by the `ledger_page_info_flag` enum.
} ledger_page_info;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get a ledger instance.
 *
 * @param ledger[out] Ledger instance.
 * @param name Ledger name.
 * @param api_version API version. Must be set to the value of `LEDGER_API_VERSION`.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_get_instance(ledger_instance** ledger, const char* name, int api_version, void* reserved);

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
 * This function can be called multiple times in the same thread with the same ledger instance.
 * In order to unlock the instance, `ledger_unlock()` needs to be called a matching number of times.
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
 * Set ledger callbacks.
 *
 * @param ledger Ledger instance.
 * @param callbacks Ledger callbacks.
 * @param user_arg User argument to be passed to the callbacks.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void ledger_set_callbacks(ledger_instance* ledger, const ledger_callbacks* callbacks, void* user_arg, void* reserved);

/**
 * Attach application-specific data to a ledger instance.
 *
 * The ledger instance takes ownership over the application data.
 *
 * @param ledger Ledger instance.
 * @param app_data Application data.
 * @param destroy Callback to be invoked to destroy the application data.
 * @param user_arg User argument to be passed to the callback.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void ledger_set_app_data(ledger_instance* ledger, void* app_data, ledger_destroy_app_data_callback destroy,
        void* user_arg, void* reserved);

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
 * Set default synchronization options.
 *
 * @param ledger Ledger instance.
 * @param options Synchronization options.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_set_default_sync_options(ledger_instance* ledger, const ledger_sync_options* options, void* reserved);

/**
 * Get a page instance.
 *
 * @param[out] page Page instance.
 * @param ledger Ledger instance.
 * @param name Page name.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_get_page(ledger_page** page, ledger_instance* ledger, const char* name, void* reserved);

/**
 * Increment a page's reference count.
 *
 * @param page Page instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void ledger_add_page_ref(ledger_page* ledger, void* reserved);

/**
 * Decrement a page's reference count.
 *
 * The page instance is destroyed when its reference count reaches 0.
 *
 * @param page Page instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void ledger_release_page(ledger_page* page, void* reserved);

/**
 * Lock a page instance.
 *
 * This function can be called multiple times in the same thread with the same page instance.
 * In order to unlock the instance, `page_unlock()` needs to be called a matching number of times.
 *
 * @param page Page instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void ledger_lock_page(ledger_page* page, void* reserved);

/**
 * Unlock a page instance.
 *
 * @param page Page instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void ledger_unlock_page(ledger_page* page, void* reserved);

/**
 * Get the instance of the ledger containing a given page.
 *
 * @param page Page instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return Ledger instance.
 */
ledger_instance* ledger_get_page_ledger(ledger_page* page, void* reserved);

/**
 * Attach application-specific data to a page instance.
 *
 * The page instance takes ownership over the application data.
 *
 * @param page Page instance.
 * @param app_data Application data.
 * @param destroy Callback to be invoked to destroy the application data.
 * @param user_arg User argument to be passed to the callback.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void ledger_set_page_app_data(ledger_page* page, void* app_data, ledger_destroy_app_data_callback destroy,
        void* user_arg, void* reserved);

/**
 * Get application-specific data associated with a page instance.
 *
 * @param page Page instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return Application data.
 */
void* ledger_get_page_app_data(ledger_page* page, void* reserved);

/**
 * Get page info.
 *
 * @param page Page instance.
 * @param[out] info Page info to be populated.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_get_page_info(ledger_page* page, ledger_page_info* info, void* reserved);

/**
 * Synchronize a page.
 *
 * @param page Page instance.
 * @param options options Synchronization options.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 if the synchronization started, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_sync_page(ledger_page* page, const ledger_sync_options* options, void* reserved);

/**
 * Unlink a page.
 *
 * @param page Page instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_unlink_page(ledger_page* page, void* reserved);

/**
 * Remove a page.
 *
 * @param page Page instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_remove_page(ledger_page* page, void* reserved);

/**
 * Open a page for reading or writing.
 *
 * @param[out] stream Stream instance.
 * @param page Page instance.
 * @param mode Flags defined by the `ledger_stream_mode` enum.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_open_page(ledger_stream** stream, ledger_page* page, int mode, void* reserved);

/**
 * Close a stream.
 *
 * @param stream Stream instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void ledger_close_stream(ledger_stream* stream, void* reserved);

/**
 * Flush a stream.
 *
 * @param stream Stream instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_flush_stream(ledger_stream* stream, void* reserved);

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

#ifdef __cplusplus
} // extern "C"
#endif
