/**
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

#include "static_assert.h"
#include "spark_wiring_string.h"
#include "spark_protocol_functions.h"
#include "system_tick_hal.h"
#include "system_defs.h"
#include "protocol_defs.h"
#include "completion_handler.h"

#include <type_traits>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include "time_compat.h"

#define DEFAULT_CLOUD_EVENT_TTL 60

namespace particle {

enum ParticleKeyErrorFlag: uint32_t
{
    NO_ERROR = 0,
    // PUBLIC_SERVER_KEY_BLANK = 1,
    PUBLIC_SERVER_KEY_CORRUPTED = 2,
    // SERVER_ADDRESS_BLANK = 4,
    SERVER_ADDRESS_CORRUPTED = 8,
    // PUBLIC_DEVICE_KEY_BLANK = 16,
    // PUBLIC_DEVICE_KEY_CORRUPTED = 32,
    // PRIVATE_DEVICE_KEY_BLANK = 64,
    // PRIVATE_DEVICE_KEY_CORRUPTED = 128
    SERVER_SETTINGS_CORRUPTED = PUBLIC_SERVER_KEY_CORRUPTED | SERVER_ADDRESS_CORRUPTED
};

const system_tick_t NOW = static_cast<system_tick_t>(-1);

} // namespace particle

typedef enum
{
	CLOUD_VAR_BOOLEAN = 1, CLOUD_VAR_INT = 2, CLOUD_VAR_STRING = 4, CLOUD_VAR_DOUBLE = 9
} Spark_Data_TypeDef;

template<typename T, typename EnableT = void>
struct CloudVariableType {
};

template<>
struct CloudVariableType<bool> {
    using ValueType = bool;
    using PointerType = const bool*;

    static const Spark_Data_TypeDef TYPE_ID = CLOUD_VAR_BOOLEAN;
};

template<typename T>
struct CloudVariableType<T, std::enable_if_t<std::is_integral<T>::value && sizeof(T) <= sizeof(int)>> {
    using ValueType = int;
    using PointerType = const int*;

    static const Spark_Data_TypeDef TYPE_ID = CLOUD_VAR_INT;
};

template<typename T>
struct CloudVariableType<T, std::enable_if_t<std::is_floating_point<T>::value && sizeof(T) <= sizeof(double)>> {
    using ValueType = double;
    using PointerType = const double*;

    static const Spark_Data_TypeDef TYPE_ID = CLOUD_VAR_DOUBLE;
};

template<>
struct CloudVariableType<const char*> {
    using ValueType = const char*;
    using PointerType = const char*;

    static const Spark_Data_TypeDef TYPE_ID = CLOUD_VAR_STRING;
};

template<>
struct CloudVariableType<char*> {
    using ValueType = const char*;
    using PointerType = char*;

    static const Spark_Data_TypeDef TYPE_ID = CLOUD_VAR_STRING;
};

template<>
struct CloudVariableType<String> {
    using ValueType = String;
    using PointerType = const String*;

    static const Spark_Data_TypeDef TYPE_ID = CLOUD_VAR_STRING;
};

typedef CloudVariableType<bool> CloudVariableTypeBool;
typedef CloudVariableType<int> CloudVariableTypeInt;
typedef CloudVariableType<double> CloudVariableTypeDouble;
typedef CloudVariableType<const char*> CloudVariableTypeString;

const CloudVariableTypeBool BOOLEAN;
const CloudVariableTypeInt INT;
const CloudVariableTypeString STRING;
const CloudVariableTypeDouble DOUBLE;

/**
 * Flags for `cloud_disconnect()`.
 */
enum CloudDisconnectFlag {
    CLOUD_DISCONNECT_GRACEFULLY = 0x01, ///< Disconnect gracefully.
    CLOUD_DISCONNECT_DONT_CLOSE = 0x02 ///< Do not close the socket.
};

/**
 * Close the cloud connection.
 *
 * @param flags Disconnection flags (a combination of flags defined by `cloud_disconnect_flag`).
 * @param cloudReason Cloud disconnection reason.
 * @param networkReason Network disconnection reason.
 * @param resetReason System reset reason.
 * @param sleepDuration Sleep duration in seconds.
 */
void cloud_disconnect(unsigned flags = 0, cloud_disconnect_reason cloudReason = CLOUD_DISCONNECT_REASON_UNKNOWN,
        network_disconnect_reason networkReason = NETWORK_DISCONNECT_REASON_NONE,
        System_Reset_Reason resetReason = RESET_REASON_NONE, unsigned sleepDuration = 0);

inline void cloud_disconnect(unsigned flags, network_disconnect_reason networkReason) {
    cloud_disconnect(flags, CLOUD_DISCONNECT_REASON_NETWORK_DISCONNECT, networkReason, RESET_REASON_NONE, 0);
}

inline void cloud_disconnect(unsigned flags, System_Reset_Reason resetReason) {
    cloud_disconnect(flags, CLOUD_DISCONNECT_REASON_SYSTEM_RESET, NETWORK_DISCONNECT_REASON_NONE, resetReason, 0);
}

#if PLATFORM_ID == PLATFORM_GCC
// avoid a c-linkage incompatible with C error on newer versions of gcc
String spark_deviceID(void);
#endif // PLATFORM_ID == PLATFORM_GCC

#ifdef __cplusplus
extern "C" {
#endif

#if PLATFORM_ID != PLATFORM_GCC
String spark_deviceID(void);
#endif // PLATFORM_ID != PLATFORM_GCC

class String;


#if defined(PLATFORM_ID)

#if !defined(UNIT_TEST) && PLATFORM_ID != PLATFORM_GCC
PARTICLE_STATIC_ASSERT(spark_data_typedef_is_1_byte, sizeof(Spark_Data_TypeDef)==1);
#endif // !defined(UNIT_TEST) && PLATFORM_ID != PLATFORM_GCC

#endif // defined(PLATFORM_ID)

const uint32_t PUBLISH_EVENT_FLAG_PUBLIC = 0x0;
const uint32_t PUBLISH_EVENT_FLAG_PRIVATE = 0x1;
const uint32_t PUBLISH_EVENT_FLAG_NO_ACK = 0x2;
const uint32_t PUBLISH_EVENT_FLAG_WITH_ACK = 0x8;
/**
 * This is a stop-gap solution until all synchronous APIs return futures, allowing asynchronous operation.
 */
const uint32_t PUBLISH_EVENT_FLAG_ASYNC = EventType::ASYNC;


PARTICLE_STATIC_ASSERT(publish_no_ack_flag_matches, PUBLISH_EVENT_FLAG_NO_ACK==EventType::NO_ACK);

typedef void (*EventHandler)(const char* name, const char* data);

typedef enum
{
  MY_DEVICES,
  ALL_DEVICES
} Spark_Subscription_Scope_TypeDef;

typedef int (*cloud_function_t)(void* data, const char* param, void* reserved);

typedef int (user_function_int_str_t)(String paramString);
typedef user_function_int_str_t* p_user_function_int_str_t;

struct  cloud_function_descriptor {
    uint16_t size;
    uint16_t padding;
    const char *funcKey;
    cloud_function_t fn;
    void* data;

     cloud_function_descriptor() {
         memset(this, 0, sizeof(*this));
         size = sizeof(*this);
     }
};

PARTICLE_STATIC_ASSERT(cloud_function_descriptor_size, sizeof(cloud_function_descriptor)==16 || sizeof(void*)!=4);

typedef struct spark_variable_t
{
    uint16_t size;
    const void* (*update)(const char* name, Spark_Data_TypeDef type, const void* var, void* reserved);

    /**
     * Copy variable data.
     *
     * The calling code takes ownership over the copied data.
     *
     * @param var Variable object.
     * @param data[out] Variable data.
     * @param size[out] Size of the variable data.
     * @return 0 on success or a negative result code in case of an error.
     */
    int (*copy)(const void* var, void** data, size_t* size);
} spark_variable_t;

/**
 * @brief Register a new variable.
 * @param varKey	The name of the variable. The length should be between 1 and USER_VAR_KEY_LENGTH bytes.
 * @param userVar	A pointer to the memory for the variable.
 * @param userVarType	The type of the variable.
 * @param extra		Additional registration details.
 * 		update	A function used to case a variable value to be computed. If defined, this is called when the variable's value is retrieved.
 */
bool spark_variable(const char *varKey, const void *userVar, Spark_Data_TypeDef userVarType, spark_variable_t* extra);

/**
 * @param funcKey   The name of the function to register. When NULL, pFunc is taken to be a
 *      cloud_function_descriptor pointer.
 * @param pFunc     The function to call, when funcKey is not null. Otherwise a cloud_function_descriptor pointer.
 * @param reserved  For future expansion, set to NULL.
 */
bool spark_function(const char *funcKey, p_user_function_int_str_t pFunc, void* reserved);

// Additional parameters for spark_send_event()
typedef struct {
    size_t size;
    completion_callback handler_callback;
    void* handler_data;
} spark_send_event_data;

/**
 * @brief Publish vitals information
 *
 * Provides a mechanism to control the interval at which system
 * diagnostic messages are sent to the cloud. Subsequently, this
 * controls the granularity of detail on the fleet health metrics.
 *
 * @param[in] period_s The period (in seconds) at which vitals messages
 *                     are to be sent to the cloud
 * @arg \p particle::NOW - A special value used to send vitals immediately
 * @arg \p 0 - Publish a final message and disable periodic publishing
 * @arg \p n - Publish an initial message and subsequent messages every \p n seconds thereafter
 * @param[in,out] reserved Reserved for future use.
 *
 * @returns \p system_error_t result code
 * @retval \p system_error_t::SYSTEM_ERROR_NONE
 * @retval \p system_error_t::SYSTEM_ERROR_IO
 *
 * @note At call time, a blocking call is made on the application thread. Any subsequent
 * timer-based calls are executed asynchronously.
 *
 */
int spark_publish_vitals(system_tick_t period_s, void *reserved);
bool spark_send_event(const char* name, const char* data, int ttl, uint32_t flags, void* reserved);
bool spark_subscribe(const char *eventName, EventHandler handler, void* handler_data,
        Spark_Subscription_Scope_TypeDef scope, const char* deviceID, void* reserved);
void spark_unsubscribe(void *reserved);
bool spark_sync_time(void *reserved);
bool spark_sync_time_pending(void* reserved);
system_tick_t spark_sync_time_last(time32_t* tm32, time_t* tm);


bool spark_process(void);
bool spark_cloud_flag_connected(void);

/**
 * Sets the auto-connect state to true. The cloud will be connected by the system.
 */
void spark_cloud_flag_connect(void);

/**
 * Sets the auto-connect state to false. The cloud will be disconnected by the system.
 */
void spark_cloud_flag_disconnect(void);

/**
 * Determines if the system will attempt to connect or disconnect from the cloud.
 */
bool spark_cloud_flag_auto_connect(void);

/**
 * Option flags for `spark_cloud_disconnect_options`.
 */
typedef enum spark_cloud_disconnect_option_flag {
    SPARK_CLOUD_DISCONNECT_OPTION_GRACEFUL = 0x01, ///< The `graceful` option is set.
    SPARK_CLOUD_DISCONNECT_OPTION_TIMEOUT = 0x02, ///< The `timeout` option is set.
    SPARK_CLOUD_DISCONNECT_OPTION_CLEAR_SESSION = 0x04 ///< The `clear_session` option is set.
} spark_cloud_disconnect_option_flag;

/**
 * Options for `spark_cloud_disconnect()`.
 */
typedef struct spark_cloud_disconnect_options {
    uint16_t size; ///< Size of this structure.
    uint8_t flags; ///< Option flags (see `spark_cloud_disconnect_option_flag`).
    uint8_t graceful; ///< Enables graceful disconnection if set to a non-zero value.
    uint32_t timeout; ///< Maximum time in milliseconds to wait for message acknowledgements.
    uint8_t clear_session; ///< Clears the session after disconnecting if set to a non-zero value.
} spark_cloud_disconnect_options;

/**
 * Disconnect from the cloud.
 *
 * @param options Options.
 * @param reserved This argument should be set to NULL.
 * @return 0 on success or a negative result code in case of an error.
 */
int spark_cloud_disconnect(const spark_cloud_disconnect_options* options, void* reserved);

ProtocolFacade* system_cloud_protocol_instance(void);

/**
 * Cloud connection properties.
 *
 * @see `spark_set_connection_property()`
 * @see `spark_get_connection_property()`
 */
typedef enum spark_connection_property {
    SPARK_CLOUD_PING_INTERVAL = 0, ///< Ping interval in milliseconds (set).
    SPARK_CLOUD_FAST_OTA_ENABLED = 1, ///< Fast OTA override (set).
    SPARK_CLOUD_DISCONNECT_OPTIONS = 2, ///< Default disconnection options (set).
    SPARK_CLOUD_MAX_EVENT_DATA_SIZE = 3, ///< Maximum size of event data (get).
    SPARK_CLOUD_MAX_VARIABLE_VALUE_SIZE = 4, ///< Maximum size of a variable value (get).
    SPARK_CLOUD_MAX_FUNCTION_ARGUMENT_SIZE = 5 ///< Maximum size of a function call argument (get).
} spark_connection_property;

int spark_set_connection_property(unsigned property, unsigned value, const void* data, void* reserved);
int spark_get_connection_property(unsigned property, void* data, size_t* size, void* reserved);

int spark_set_random_seed_from_cloud_handler(void (*handler)(unsigned int), void* reserved);

#define SPARK_BUF_LEN                 600

//#define SPARK_SERVER_IP             "54.235.79.249"
#define SPARK_SERVER_PORT             5683
#define PORT_COAPS                    (5684)
#define SPARK_LOOP_DELAY_MILLIS       1000    //1sec
#define SPARK_RECEIVE_DELAY_MILLIS    10      //10ms

#define TIMING_FLASH_UPDATE_TIMEOUT   (300000) // 300sec

#define USER_VAR_MAX_COUNT            (100)
#define USER_FUNC_MAX_COUNT           (100)

#define USER_FUNC_ARG_LENGTH      (622) // FIXME: NOT USED
#define USER_VAR_KEY_LENGTH       (64)
#define USER_FUNC_KEY_LENGTH      (64)
#define USER_EVENT_NAME_LENGTH    (64)  // FIXME: NOT USED
#define USER_EVENT_DATA_LENGTH    (622) // FIXME: NOT USED

#ifdef __cplusplus
}
#endif
