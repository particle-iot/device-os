/**
  ******************************************************************************
  * @file    spark_descriptor.h
  * @authors  Zachary Crockett
  * @version V1.0.0
  * @date    15-Nov-2013
  * @brief   SPARK DESCRIPTOR
  ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
  */

#pragma once

#include "appender.h"
#include "static_assert.h"
#include "events.h"
#include <functional>

// Deferring to ASN.1 type codes
namespace SparkReturnType {
  enum Enum {
    BOOLEAN = 1,
    INT     = 2,
    STRING  = 4,
    DOUBLE  = 9
  };
}

/**
 * The application state is divided into distinct types.
 */
namespace SparkAppStateSelector {
	enum Enum {
		DESCRIBE_APP = 0,
		DESCRIBE_SYSTEM = 1,
		SUBSCRIPTIONS = 2,
		PROTOCOL_FLAGS = 3,
		ALL = 4,
		SYSTEM_MODULE_VERSION = 5,
		MAX_MESSAGE_SIZE = 6,
		MAX_BINARY_SIZE = 7,
		OTA_CHUNK_SIZE = 8
	};
}

namespace SparkAppStateUpdate {
	enum Enum {
		COMPUTE = 1,
		PERSIST = 2,
		COMPUTE_AND_PERSIST = 3,
		RESET = 4
	};
}

struct SparkDescriptor
{
    typedef std::function<bool(const void*, SparkReturnType::Enum)> FunctionResultCallback;

    /**
     * A callback invoked when the processing of a variable request is completed.
     *
     * @param error Result code (a value defined by the `ProtocolError` enum).
     * @param type Variable type (a value defined by the `SparkReturnType::Enum` enum).
     * @param data Variable value. The ownership over the allocated memory is transferred to the callback.
     * @param size Size of the variable value.
     * @param context Context of the variable request.
     */
    typedef void (*GetVariableCallback)(int error, int type, void* data, size_t size, void* context);

    size_t size;
    int (*num_functions)(void); // Deprecated
    const char* (*get_function_key)(int function_index); // Deprecated
    int (*call_function)(const char *function_key, const char *arg, FunctionResultCallback callback, void* reserved);

    int (*num_variables)(void); // Deprecated
    const char* (*get_variable_key)(int variable_index); // Deprecated
    SparkReturnType::Enum (*variable_type)(const char *variable_key); // Deprecated
    const void *(*get_variable)(const char *variable_key); // Deprecated

    bool (*was_ota_upgrade_successful)(void);
    void (*ota_upgrade_status_sent)(void);

    bool (*append_system_info)(appender_fn appender, void* append, void* reserved);

    void (*call_event_handler)(uint16_t size, FilteringEventHandler* handler, const char* event, const char* data, void* reserved);

    /**
     * Optional callback - may be null.
     * @param selector	The app state information to retrieve or update
     * @param operation	COMPUTE to retrieve, the value. PESIST to set the persistent storage to the given value, COMPUTE_AND_PERSIST to compute and persist a given value. funcs/vars crc can be retrieved,
     * 	subscriptions crc can be set.
     * 	The descriptor state (DESCRIBE_APP/DESCRIBE_SYSTEM) can be computed by the callback and can be used with COMPUTE and COMPUTE_AND_PERSIST operations.
     * 	The subscription state (SUBSCRIPTIONS) is computed by the caller and passed to the callback (secifying PERSIST as the operation.)
     * @param data		when operation==1 this is the value ot set. otherwise unused.
     * @return when operation==COMPUTE, the crc of the application state is retrieved when operation is COMPUTE. Otherwise the return value is 0.
     */
    uint32_t (*app_state_selector_info)(SparkAppStateSelector::Enum selector, SparkAppStateUpdate::Enum operation, uint32_t data, void* reserved);

    /**
     * Append metrics to the given appender.
     * @param appender	The appender function to call with the "append" data and the string to append
     * @param append		Opaque data to be passed to appender
     * @param flags		0x01 - append as binary daata, otherwise append as json
     * @param page		A key to select which metrics data to output. Presently unused and should be 0, which means the default metrics.
     * @param reserved	For future expansion.
     * @return
     */
    bool (*append_metrics)(appender_fn appender, void* append, uint32_t flags, uint32_t page, void* reserved);

    /**
     * Get the value of a variable asynchronously.
     *
     * @param key Variable name.
     * @param callback Completion callback.
     * @param context Context of the variable request. This argument needs to be passed to the completion callback.
     */
    void (*get_variable_async)(const char* key, GetVariableCallback callback, void* context);

    /**
     * Serialize application info using the given appender function.
     *
     * @param appender Appender function.
     * @param append Opaque data to be passed to appender function.
     * @param reserved Reserved argument.
     * @return true on success or false on failure.
     */
    bool (*append_app_info)(appender_fn appender, void* append, void* reserved);
};

PARTICLE_STATIC_ASSERT(SparkDescriptor_size, sizeof(SparkDescriptor)==64 || sizeof(void*)!=4);
