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
#include "completion_handler.h"
#include <string.h>
#include <time.h>
#include <stdint.h>

enum ParticleKeyErrorFlag: uint32_t
{
  NO_ERROR                      = 0,
  PUBLIC_SERVER_KEY_BLANK       = 1,
  PUBLIC_SERVER_KEY_CORRUPTED   = 2,
  SERVER_ADDRESS_BLANK          = 4,
  SERVER_ADDRESS_CORRUPTED      = 8,
  PUBLIC_DEVICE_KEY_BLANK       = 16,
  PUBLIC_DEVICE_KEY_CORRUPTED   = 32,
  PRIVATE_DEVICE_KEY_BLANK      = 64,
  PRIVATE_DEVICE_KEY_CORRUPTED  = 128
};

typedef enum
{
	CLOUD_VAR_BOOLEAN = 1, CLOUD_VAR_INT = 2, CLOUD_VAR_STRING = 4, CLOUD_VAR_DOUBLE = 9
} Spark_Data_TypeDef;

namespace particle {
    static const system_tick_t NOW = static_cast<system_tick_t>(-1);
}

struct CloudVariableTypeBase {};
struct CloudVariableTypeBool : public CloudVariableTypeBase {
    using vartype = bool;
    using varref = const bool*;
    CloudVariableTypeBool(){};
    static inline Spark_Data_TypeDef value() { return CLOUD_VAR_BOOLEAN; }
};
struct CloudVariableTypeInt : public CloudVariableTypeBase {
    using vartype = int;
    using varref = const int*;
    CloudVariableTypeInt(){};
    static inline Spark_Data_TypeDef value() { return CLOUD_VAR_INT; }
};
struct CloudVariableTypeString : public CloudVariableTypeBase {
    using vartype = const char*;
    using varref = const char*;
    CloudVariableTypeString(){};
    static inline Spark_Data_TypeDef value() { return CLOUD_VAR_STRING; }
};
struct CloudVariableTypeDouble : public CloudVariableTypeBase {
    using vartype = double;
    using varref = const double*;

    CloudVariableTypeDouble(){};
    static inline Spark_Data_TypeDef value() { return CLOUD_VAR_DOUBLE; }
};

const CloudVariableTypeBool BOOLEAN;
const CloudVariableTypeInt INT;
const CloudVariableTypeString STRING;
const CloudVariableTypeDouble DOUBLE;

#if PLATFORM_ID==3
// avoid a c-linkage incompatible with C error on newer versions of gcc
String spark_deviceID(void);
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum cloud_disconnect_reason {
    CLOUD_DISCONNECT_REASON_NONE = 0,
    CLOUD_DISCONNECT_REASON_ERROR = 1, // Disconnected due to an error
    CLOUD_DISCONNECT_REASON_USER = 2, // Disconnected at the user's request
    CLOUD_DISCONNECT_REASON_NETWORK_DISCONNECT = 3, // Disconnected due to the network disconnection
    CLOUD_DISCONNECT_REASON_LISTENING = 4 // Disconnected due to the listening mode
} cloud_disconnect_reason;

#if PLATFORM_ID!=3
String spark_deviceID(void);
#endif

void cloud_disconnect(bool closeSocket=true, bool graceful=false, cloud_disconnect_reason reason = CLOUD_DISCONNECT_REASON_NONE);
void cloud_disconnect_graceful(bool closeSocket=true, cloud_disconnect_reason reason = CLOUD_DISCONNECT_REASON_NONE);

class String;


#if defined(PLATFORM_ID)

#if PLATFORM_ID!=3 && PLATFORM_ID != 20
PARTICLE_STATIC_ASSERT(spark_data_typedef_is_1_byte, sizeof(Spark_Data_TypeDef)==1);
#endif

#endif

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
    const void* (*update)(const char* nane, Spark_Data_TypeDef type, const void* var, void* reserved);
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
 * @arg \p particle::NOW - Special value used to send vitals immediately
 * @arg \p 0 - Publish a final message and disable periodic publishing
 * @arg \p n - Publish an initial message and subsequent messages every \p n seconds thereafter
 * @param[in,out] reserved Reserved for future use.
 *
 * @returns \p system_error_t result code
 * @retval \p system_error_t::SYSTEM_ERROR_NONE
 * @retval \p system_error_t::SYSTEM_ERROR_IO
 *
 * @note The periodic functionality is not available for the Spark Core
 */
int spark_publish_vitals(system_tick_t period_s, void *reserved);
bool spark_send_event(const char* name, const char* data, int ttl, uint32_t flags, void* reserved);
bool spark_subscribe(const char *eventName, EventHandler handler, void* handler_data,
        Spark_Subscription_Scope_TypeDef scope, const char* deviceID, void* reserved);
void spark_unsubscribe(void *reserved);
bool spark_sync_time(void *reserved);
bool spark_sync_time_pending(void* reserved);
system_tick_t spark_sync_time_last(time_t* tm, void* reserved);


void spark_process(void);
bool spark_cloud_flag_connected(void);

/**
 * Sets the auto-connect state to true. The cloud will be connected by the system.
 */
void spark_cloud_flag_connect(void);

/**
 * Sets the auto-connect state to false. The cloud will be disconnected by the system.
 */
void spark_cloud_flag_disconnect(void);    // should be set connected since it manages the connection state)

/**
 * Determines if the system will attempt to connect or disconnect from the cloud.
 */
bool spark_cloud_flag_auto_connect(void);

ProtocolFacade* system_cloud_protocol_instance(void);

int spark_set_connection_property(unsigned property_id, unsigned data, particle::protocol::connection_properties_t* conn_prop, void* reserved);

int spark_set_random_seed_from_cloud_handler(void (*handler)(unsigned int), void* reserved);

extern const unsigned char backup_udp_public_server_key[];
extern const size_t backup_udp_public_server_key_size;

extern const unsigned char backup_udp_public_server_address[];
extern const size_t backup_udp_public_server_address_size;

extern const unsigned char backup_tcp_public_server_key[294];
extern const unsigned char backup_tcp_public_server_address[18];

#define SPARK_BUF_LEN                 600

//#define SPARK_SERVER_IP             "54.235.79.249"
#define SPARK_SERVER_PORT             5683
#define PORT_COAPS                    (5684)
#define SPARK_LOOP_DELAY_MILLIS       1000    //1sec
#define SPARK_RECEIVE_DELAY_MILLIS    10      //10ms

#if PLATFORM_ID==10 || HAL_PLATFORM_MESH
#define TIMING_FLASH_UPDATE_TIMEOUT   (300000) // 300sec
#else
#define TIMING_FLASH_UPDATE_TIMEOUT   (30000)  // 30sec
#endif

#define USER_VAR_MAX_COUNT            (10)  // FIXME: NOT USED
#define USER_FUNC_MAX_COUNT           (4)   // FIXME: NOT USED

#if PLATFORM_ID<2
    #define USER_FUNC_ARG_LENGTH      (64)  // FIXME: NOT USED
    #define USER_VAR_KEY_LENGTH       (12)
    #define USER_FUNC_KEY_LENGTH      (12)
    #define USER_EVENT_NAME_LENGTH    (64)  // FIXME: NOT USED
    #define USER_EVENT_DATA_LENGTH    (64)  // FIXME: NOT USED
#else
    #define USER_FUNC_ARG_LENGTH      (622) // FIXME: NOT USED
    #define USER_VAR_KEY_LENGTH       (64)
    #define USER_FUNC_KEY_LENGTH      (64)
    #define USER_EVENT_NAME_LENGTH    (64)  // FIXME: NOT USED
    #define USER_EVENT_DATA_LENGTH    (622) // FIXME: NOT USED
#endif

#ifdef __cplusplus
}
#endif
