/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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
 * Maximum length of an event name.
 */
#define CLOUD_EVENT_MAX_NAME_LENGTH 64

/**
 * Event instance.
 */
typedef struct cloud_event cloud_event;

/**
 * Callback invoked when the status of the event changes.
 *
 * @param event Event instance.
 * @param arg User data.
 */
typedef void (*cloud_event_status_change_callback)(cloud_event* event, void* arg);

/**
 * Callback invoked when an event is received for a subscription.
 *
 * @param event Event instance.
 * @param arg User data.
 */
typedef void (*cloud_event_subscribe_callback)(cloud_event* event, void* arg);

/**
 * Destructor callback.
 *
 * @param arg User data.
 */
typedef void (*cloud_event_destroy_callback)(void* arg);

/**
 * Event status.
 */
typedef enum cloud_event_status {
    CLOUD_EVENT_STATUS_NEW = 1, ///< Reading/writing the event data.
    CLOUD_EVENT_STATUS_SENDING = 2, ///< The event is being sent.
    CLOUD_EVENT_STATUS_SENT = 3, ///< The event was sent successfully. 
    CLOUD_EVENT_STATUS_FAILED = 4 ///< An error occured while reading, writing or sending the event.
} cloud_event_status;

/**
 * Event property flag.
 */
typedef enum cloud_event_property {
    CLOUD_EVENT_PROPERTY_CONTENT_TYPE = 0x01, ///< Content type of the event data.
    CLOUD_EVENT_PROPERTY_ALL = CLOUD_EVENT_PROPERTY_CONTENT_TYPE ///< All properies.
} cloud_event_property;

/**
 * Event properties.
 */
typedef struct cloud_event_properties {
    /**
     * Combination of flags defined by `cloud_event_property`.
     *
     * The value of this field describes what properties of the event are being set or queried.
     * For each property flag set, the corresponding field of this structure must contain a valid
     * value of the property.
     */
    int flags;
    /**
     * Content type of the event data (`CLOUD_EVENT_PROPERTY_CONTENT_TYPE`).
     */
    int content_type;
} cloud_event_properties;

/**
 * Options for `cloud_event_publish()`.
 */
typedef struct cloud_event_publish_options cloud_event_publish_options;

/**
 * Options for `cloud_event_subscribe()`.
 */
typedef struct cloud_event_subscribe_options cloud_event_subscribe_options;

#ifdef __cplusplus
extern "C" {
#endif

int cloud_event_create(cloud_event** event, void* reserved);

void cloud_event_add_ref(cloud_event* event, void* reserved);
void cloud_event_release(cloud_event* event, void* reserved);

int cloud_event_set_name(cloud_event* event, const char* name, void* reserved);
const char* cloud_event_get_name(cloud_event* event, void* reserved);

int cloud_event_set_properties(cloud_event* event, const cloud_event_properties* properties, void* reserved);
int cloud_event_get_properties(cloud_event* event, cloud_event_properties* properties, void* reserved);

int cloud_event_read(cloud_event* event, char* data, size_t size, void* reserved);
int cloud_event_peek(cloud_event* event, char* data, size_t size, void* reserved);
int cloud_event_write(cloud_event* event, const char* data, size_t size, void* reserved);
int cloud_event_seek(cloud_event* event, size_t pos, void* reserved);
int cloud_event_tell(cloud_event* event, void* reserved);

int cloud_event_set_size(cloud_event* event, size_t size, void* reserved);
int cloud_event_get_size(cloud_event* event, void* reserved);

void cloud_event_set_status_change_callback(cloud_event* event, cloud_event_status_change_callback status_change,
        cloud_event_destroy_callback destroy, void* arg, void* reserved);
int cloud_event_get_status(cloud_event* event, void* reserved);

void cloud_event_set_error(cloud_event* event, int error, void* reserved);
int cloud_event_get_error(cloud_event* event, void* reserved);
void cloud_event_clear_error(cloud_event* event, void* reserved);

int cloud_event_publish(cloud_event* event, const cloud_event_publish_options* opts, void* reserved);

int cloud_event_subscribe(const char* prefix, cloud_event_subscribe_callback subscribe, cloud_event_destroy_callback destroy,
        void* arg, const cloud_event_subscribe_options* opts, void* reserved);
void cloud_event_unsubscribe(const char* prefix, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif
