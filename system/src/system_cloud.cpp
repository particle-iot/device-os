/**
 ******************************************************************************
 * @file    system_cloud.cpp
 * @author  Satish Nair, Zachary Crockett, Mohit Bhoite, Matthew McGowan
 * @version V1.0.0
 * @date    13-March-2013
 *
 * Updated: 14-Feb-2014 David Sidrane <david_s5@usa.net>
 * @brief
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

#include "spark_wiring_string.h"
#include "system_cloud.h"
#include "system_cloud_internal.h"
#include "system_task.h"
#include "system_threading.h"
#include "system_update.h"
#include "system_cloud_internal.h"
#include "string_convert.h"
#include "spark_protocol_functions.h"
#include "events.h"
#include "deviceid_hal.h"
#include "system_mode.h"


#ifndef SPARK_NO_CLOUD

SubscriptionScope::Enum convert(Spark_Subscription_Scope_TypeDef subscription_type)
{
    return(subscription_type==MY_DEVICES) ? SubscriptionScope::MY_DEVICES : SubscriptionScope::FIREHOSE;
}

bool register_event(const char* eventName, SubscriptionScope::Enum event_scope, const char* deviceID)
{
    bool success;
    if (deviceID)
        success = spark_protocol_send_subscription_device(sp, eventName, deviceID);
    else
        success = spark_protocol_send_subscription_scope(sp, eventName, event_scope);
    return success;
}

bool spark_subscribe(const char *eventName, EventHandler handler, void* handler_data,
        Spark_Subscription_Scope_TypeDef scope, const char* deviceID, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_SYNC(spark_subscribe(eventName, handler, handler_data, scope, deviceID, reserved));
    auto event_scope = convert(scope);
    bool success = spark_protocol_add_event_handler(sp, eventName, handler, event_scope, deviceID, handler_data);
    if (success && spark_cloud_flag_connected())
    {
        register_event(eventName, event_scope, deviceID);
    }
    return success;
}

/**
 * Convert from the API flags to the communications lib flags
 * The event visibility flag (public/private) is encoded differently. The other flags map directly.
 */
inline uint32_t convert(uint32_t flags) {
	bool priv = flags & PUBLISH_EVENT_FLAG_PRIVATE;
	flags &= ~PUBLISH_EVENT_FLAG_PRIVATE;
	flags |= !priv ? EventType::PUBLIC : EventType::PRIVATE;
	return flags;
}

bool spark_send_event(const char* name, const char* data, int ttl, uint32_t flags, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_SYNC(spark_send_event(name, data, ttl, flags, reserved));

    return spark_protocol_send_event(sp, name, data, ttl, convert(flags), NULL);
}

bool spark_variable(const char *varKey, const void *userVar, Spark_Data_TypeDef userVarType, spark_variable_t* extra)
{
    SYSTEM_THREAD_CONTEXT_SYNC(spark_variable(varKey, userVar, userVarType, extra));

    User_Var_Lookup_Table_t* item = NULL;
    if (NULL != userVar && NULL != varKey && strlen(varKey)<=USER_VAR_KEY_LENGTH)
    {
        if ((item=find_var_by_key_or_add(varKey))!=NULL)
        {
            item->userVar = userVar;
            item->userVarType = userVarType;
            if (extra) {
                item->update = extra->update;
            }
            memset(item->userVarKey, 0, USER_VAR_KEY_LENGTH);
            memcpy(item->userVarKey, varKey, USER_VAR_KEY_LENGTH);
        }
    }
    return item!=NULL;
}

/**
 * This is the original released signature for firmware version 0 and needs to remain like this.
 * (The original returned void - we can safely change to bool.)
 */
bool spark_function(const char *funcKey, p_user_function_int_str_t pFunc, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_SYNC(spark_function(funcKey, pFunc, reserved));

    bool result;
    if (funcKey) {                          // old call, with funcKey != NULL
        cloud_function_descriptor desc;
        desc.funcKey = funcKey;
        desc.fn = call_raw_user_function;
        desc.data = (void*)pFunc;
        result = spark_function_internal(&desc, NULL);
    }
    else {      // new call - pFunc is actually a pointer to a descriptor
        result = spark_function_internal((cloud_function_descriptor*)pFunc, reserved);
    }
    return result;
}

#endif

bool spark_cloud_flag_connected(void)
{
    if (SPARK_CLOUD_SOCKETED && SPARK_CLOUD_CONNECTED)
        return true;
    else
        return false;
}

void spark_process(void)
{
	// application thread will pump application messages
#if PLATFORM_THREADING
    if (system_thread_get_state(NULL) && APPLICATION_THREAD_CURRENT())
    {
        ApplicationThread.process();
        return;
    }
#endif

    // run the background processing loop, and specifically also pump cloud events
    Spark_Idle_Events(true);
}

String spark_deviceID(void)
{
    unsigned len = HAL_device_ID(NULL, 0);
    uint8_t id[len];
    HAL_device_ID(id, len);
    return bytes2hex(id, len);
}
