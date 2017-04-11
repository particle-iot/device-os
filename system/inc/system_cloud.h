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
#include "completion_handler.h"
#include <string.h>
#include <time.h>
#include <stdint.h>

#include "spark_wiring_flags.h"

using particle::Flags;

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

PARTICLE_DEFINE_FLAG_OPERATORS(ParticleKeyErrorFlag)

typedef enum
{
	CLOUD_VAR_BOOLEAN = 1, CLOUD_VAR_INT = 2, CLOUD_VAR_STRING = 4, CLOUD_VAR_DOUBLE = 9
} Spark_Data_TypeDef;

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
#if PLATFORM_ID!=3
String spark_deviceID(void);
#endif

void cloud_disconnect(bool closeSocket=true);


class String;


#if defined(PLATFORM_ID)

#if PLATFORM_ID!=3
STATIC_ASSERT(spark_data_typedef_is_1_byte, sizeof(Spark_Data_TypeDef)==1);
#endif

#endif

const uint32_t PUBLISH_EVENT_FLAG_PUBLIC = 0x0;
const uint32_t PUBLISH_EVENT_FLAG_PRIVATE = 0x1;
const uint32_t PUBLISH_EVENT_FLAG_NO_ACK = 0x2;
const uint32_t PUBLISH_EVENT_FLAG_WITH_ACK = 0x8;

STATIC_ASSERT(publish_no_ack_flag_matches, PUBLISH_EVENT_FLAG_NO_ACK==EventType::NO_ACK);

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

STATIC_ASSERT(cloud_function_descriptor_size, sizeof(cloud_function_descriptor)==16 || sizeof(void*)!=4);

typedef struct spark_variable_t
{
    uint16_t size;
    const void* (*update)(const char* nane, Spark_Data_TypeDef type, const void* var, void* reserved);
} spark_variable_t;

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

int spark_set_connection_property(unsigned property_id, unsigned data, void* datap, void* reserved);

// minimal udp public server key
const unsigned char backup_udp_public_server_key[] = {
  0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02,
  0x01, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03,
  0x42, 0x00, 0x04, 0x50, 0x9b, 0xfc, 0x18, 0x56, 0x48, 0xc3, 0x3f, 0x80,
  0x90, 0x7a, 0xe1, 0x32, 0x60, 0xdf, 0x33, 0x28, 0x21, 0x15, 0x20, 0x9e,
  0x54, 0xa2, 0x2f, 0x2b, 0x10, 0x59, 0x84, 0xa4, 0x63, 0x62, 0xc0, 0x7c,
  0x26, 0x79, 0xf6, 0xe4, 0xce, 0x76, 0xca, 0x00, 0x2d, 0x3d, 0xe4, 0xbf,
  0x2e, 0x9e, 0x3a, 0x62, 0x15, 0x1c, 0x48, 0x17, 0x9b, 0xd8, 0x09, 0xdd,
  0xce, 0x9c, 0x5d, 0xc3, 0x0f, 0x54, 0xb8
};
const unsigned char backup_udp_public_server_address[] = {
  0x01, 0x13, 0x24, 0x69, 0x64, 0x2e, 0x75, 0x64, 0x70, 0x2e, 0x70, 0x61,
  0x72, 0x74, 0x69, 0x63, 0x6c, 0x65, 0x2e, 0x69, 0x6f, 0x00
};
STATIC_ASSERT(backup_udp_public_server_key_size, sizeof(backup_udp_public_server_key)==91);
STATIC_ASSERT(backup_udp_public_server_address_size, sizeof(backup_udp_public_server_address)==22);

// minimal tcp public server key
const unsigned char backup_tcp_public_server_key[] = {
  0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86,
  0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0f, 0x00,
  0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01, 0x00, 0xbe, 0xcc, 0xbe,
  0x43, 0xdb, 0x8e, 0xea, 0x15, 0x27, 0xa6, 0xbb, 0x52, 0x6d, 0xe1, 0x51,
  0x2b, 0xa0, 0xab, 0xcc, 0xa1, 0x64, 0x77, 0x48, 0xad, 0x7c, 0x66, 0xfc,
  0x80, 0x7f, 0xf6, 0x99, 0xa5, 0x25, 0xf2, 0xf2, 0xda, 0xe0, 0x43, 0xcf,
  0x3a, 0x26, 0xa4, 0x9b, 0xa1, 0x87, 0x03, 0x0e, 0x9a, 0x8d, 0x23, 0x9a,
  0xbc, 0xea, 0x99, 0xea, 0x68, 0xd3, 0x5a, 0x14, 0xb1, 0x26, 0x0f, 0xbd,
  0xaa, 0x6d, 0x6f, 0x0c, 0xac, 0xc4, 0x77, 0x2c, 0xd1, 0xc5, 0xc8, 0xb1,
  0xd1, 0x7b, 0x68, 0xe0, 0x25, 0x73, 0x7b, 0x52, 0x89, 0x68, 0x20, 0xbd,
  0x06, 0xc6, 0xf0, 0xe6, 0x00, 0x30, 0xc0, 0xe0, 0xcf, 0xf6, 0x1b, 0x3a,
  0x45, 0xe9, 0xc4, 0x5b, 0x55, 0x17, 0x06, 0xa3, 0xd3, 0x4a, 0xc6, 0xd5,
  0xb8, 0xd2, 0x17, 0x02, 0xb5, 0x27, 0x7d, 0x8d, 0xe4, 0xd4, 0x7d, 0xd3,
  0xed, 0xc0, 0x1d, 0x8a, 0x7c, 0x25, 0x1e, 0x21, 0x4a, 0x51, 0xae, 0x57,
  0x06, 0xdd, 0x60, 0xbc, 0xa1, 0x34, 0x90, 0xaa, 0xcc, 0x09, 0x9e, 0x3b,
  0x3a, 0x41, 0x4c, 0x3c, 0x9d, 0xf3, 0xfd, 0xfd, 0xb7, 0x27, 0xc1, 0x59,
  0x81, 0x98, 0x54, 0x60, 0x4a, 0x62, 0x7a, 0xa4, 0x9a, 0xbf, 0xdf, 0x92,
  0x1b, 0x3e, 0xfc, 0xa7, 0xe4, 0xa4, 0xb3, 0x3a, 0x9a, 0x5f, 0x57, 0x93,
  0x8e, 0xeb, 0x19, 0x64, 0x95, 0x22, 0x4a, 0x2c, 0xd5, 0x60, 0xf5, 0xf9,
  0xd0, 0x03, 0x50, 0x83, 0x69, 0xc0, 0x6b, 0x53, 0xf0, 0xf0, 0xda, 0xf8,
  0x13, 0x82, 0x1f, 0xcc, 0xbb, 0x5f, 0xe2, 0xc1, 0xdf, 0x3a, 0xe9, 0x7f,
  0x5d, 0xe2, 0x7d, 0xb9, 0x50, 0x80, 0x3c, 0x58, 0x33, 0xef, 0x8c, 0xf3,
  0x80, 0x3f, 0x11, 0x01, 0xd2, 0x68, 0x86, 0x5f, 0x3c, 0x5e, 0xe6, 0xc1,
  0x8e, 0x32, 0x2b, 0x28, 0xcb, 0xb5, 0xcc, 0x1b, 0xa8, 0x50, 0x5e, 0xa7,
  0x0d, 0x02, 0x03, 0x01, 0x00, 0x01
};
const unsigned char backup_tcp_public_server_address[] = {
  0x01, 0x0f, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x2e, 0x73, 0x70, 0x61,
  0x72, 0x6b, 0x2e, 0x69, 0x6f, 0x00
};
STATIC_ASSERT(backup_tcp_public_server_key_size, sizeof(backup_tcp_public_server_key)==294);
STATIC_ASSERT(backup_tcp_public_server_address_size, sizeof(backup_tcp_public_server_address)==18);

#define SPARK_BUF_LEN                 600

//#define SPARK_SERVER_IP             "54.235.79.249"
#define SPARK_SERVER_PORT             5683
#define PORT_COAPS                    (5684)
#define SPARK_LOOP_DELAY_MILLIS       1000    //1sec
#define SPARK_RECEIVE_DELAY_MILLIS    10      //10ms

#if PLATFORM_ID==10
#define TIMING_FLASH_UPDATE_TIMEOUT   90000   //90sec
#else
#define TIMING_FLASH_UPDATE_TIMEOUT   30000   //30sec
#endif

#define USER_VAR_MAX_COUNT            10
#define USER_VAR_KEY_LENGTH           12

#define USER_FUNC_MAX_COUNT           4
#define USER_FUNC_KEY_LENGTH          12
#define USER_FUNC_ARG_LENGTH          64

#define USER_EVENT_NAME_LENGTH        64
#define USER_EVENT_DATA_LENGTH        64

#ifdef __cplusplus
}
#endif
