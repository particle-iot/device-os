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
#include <string.h>
#include <time.h>
#include <stdint.h>

typedef class SparkProtocol SparkProtocol;


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


#ifdef __cplusplus
extern "C" {
#endif

String spark_deviceID(void);

void cloud_disconnect(bool closeSocket=true);


class String;


#if defined(PLATFORM_ID)

#if PLATFORM_ID!=3
STATIC_ASSERT(spark_data_typedef_is_1_byte, sizeof(Spark_Data_TypeDef)==1);
#endif

#endif


typedef enum
{
	PUBLIC = 0, PRIVATE = 1
} Spark_Event_TypeDef;

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
bool spark_send_event(const char* name, const char* data, int ttl, Spark_Event_TypeDef eventType, void* reserved);
bool spark_subscribe(const char *eventName, EventHandler handler, void* handler_data,
        Spark_Subscription_Scope_TypeDef scope, const char* deviceID, void* reserved);


void spark_process(void);
void spark_connect(void);
void spark_disconnect(void);    // should be set connected since it manages the connection state)
bool spark_connected(void);
SparkProtocol* system_cloud_protocol_instance(void);


#define SPARK_BUF_LEN			        600

//#define SPARK_SERVER_IP			        "54.235.79.249"
#define SPARK_SERVER_PORT		        5683

#define SPARK_LOOP_DELAY_MILLIS		        1000    //1sec
#define SPARK_RECEIVE_DELAY_MILLIS              10      //10ms

#define TIMING_FLASH_UPDATE_TIMEOUT             30000   //30sec

#define USER_VAR_MAX_COUNT		        10
#define USER_VAR_KEY_LENGTH		        12

#define USER_FUNC_MAX_COUNT		        4
#define USER_FUNC_KEY_LENGTH		        12
#define USER_FUNC_ARG_LENGTH		        64

#define USER_EVENT_NAME_LENGTH		        64
#define USER_EVENT_DATA_LENGTH		        64

#ifdef __cplusplus
}
#endif
