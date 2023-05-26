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

/**
 * Ledger API version.
 */
#define LEDGER_API_VERSION 1

/**
 * Ledger instance.
 */
struct ledger_handle;
typedef struct ledger_handle ledger_handle;

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
 * @param ladger Ledger instance.
 * @param page_name Page name.
 * @param error 0 if the synchronization succeeded, otherwise an error code defined by the `system_error_t` enum.
 * @param user_data User data provided when the callback was registered.
 */
typedef void (*ledger_page_sync_callback)(ledger_handle* ledger, const char* page_name, int error, void* user_data);

/**
 * Callback invoked when a page has changed in the Cloud.
 *
 * @param ladger Ledger instance.
 * @param page_name Page name.
 * @param flags Flags defined by the `ledger_page_change_flag` enum.
 * @param user_data User data provided when the callback was registered.
 */
typedef void (*ledger_page_change_callback)(ledger_handle* ledger, const char* page_name, int flags, void* user_data);

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
    LEDGER_PAGE_INFO_FLAG_CHANGED = 0x01, ///< Page has changes that have not yet been synchronized with the Cloud.
    LEDGER_PAGE_INFO_FLAG_SYNCING = 0x02 ///< Synchronization is in progress for this page.
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
    LEDGER_SYNC_STRATEGY_DEFAULT = 0, ///< Default strategy.
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
 * Ledger info.
 */
typedef struct ledger_info {
    const char* name; ///< Ledger name.
    int scope; ///< Ledger scope.
} ledger_info;

/**
 * Page info.
 */
typedef struct ledger_page_info {
    const char* name; ///< Page name.
    size_t data_size; ///< Size of the page contents.
    const char* checksum; ///< Checksum of the page contents.
    size_t checksum_size; ///< Size of the checksum.
    int flags; ///< Flags defined by the `ledger_page_info_flag` enum.
} ledger_page_info;

/**
 * Synchronization options.
 */
typedef struct ledger_sync_options {
    int strategy; ///< Synchronization strategy as defined by the `ledger_sync_strategy` enum.
} ledger_sync_options;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open a ledger.
 *
 * @param ledger[out] Ledger instance.
 * @param name Ledger name.
 * @param api_version API version. Must be set to the value of `LEDGER_API_VERSION`.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_open(ledger_handle** ledger, const char* name, int api_version, void* reserved);

/**
 * Close a ledger.
 *
 * @param ledger Ledger instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void ledger_close(ledger_handle* ledger, void* reserved);

/**
 * Get ledger info.
 *
 * @param ledger Ledger instance.
 * @param[out] info Ledger info to be populated.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_get_info(ledger_handle* ledger, ledger_info* info, void* reserved);

/**
 * Set ledger callbacks.
 *
 * @param ledger Ledger instance.
 * @param callbacks Ledger callbacks.
 * @param user_data User data to be passed to the callbacks.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_set_callbacks(ledger_handle* ledger, const ledger_callbacks* callbacks, void* user_data, void* reserved);

/**
 * Set default synchronization options.
 *
 * @param ledger Ledger instance.
 * @param options Synchronization options.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_set_default_sync_options(ledger_handle* ledger, const ledger_sync_options* options, void* reserved);

/**
 * Open a page.
 *
 * @param[out] page Page instance.
 * @param ledger Ledger instance.
 * @param name Page name.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_open_page(ledger_page_handle** page, ledger_handle* ledger, const char* name, void* reserved);

/**
 * Close a page.
 *
 * @param page Page instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 */
void ledger_close_page(ledger_page_handle* page, void* reserved);

/**
 * Get page info.
 *
 * @param page Page instance.
 * @param[out] info Page info to be populated.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_get_page_info(ledger_page_handle* page, ledger_page_info* info, void* reserved);

/**
 * Synchronize a page.
 *
 * @param page Page instance.
 * @param options options Synchronization options.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 if the synchronization started, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_sync_page(ledger_page_handle* page, const ledger_sync_options* options, void* reserved);

/**
 * Unlink a page.
 *
 * @param page Page instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_unlink_page(ledger_page_handle* page, void* reserved);

/**
 * Remove a page.
 *
 * @param page Page instance.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_remove_page(ledger_page_handle* page, void* reserved);

/**
 * Open a page for reading or writing.
 *
 * @param[out] stream Stream instance.
 * @param page Page instance.
 * @param mode Flags defined by the `ledger_stream_mode` enum.
 * @param reserved Reserved argument. Must be set to `NULL`.
 * @return 0 on success, otherwise an error code defined by the `system_error_t` enum.
 */
int ledger_open_page_stream(ledger_stream** stream, ledger_page_handle* page, int mode, void* reserved);

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
