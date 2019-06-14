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

#include <cstdarg>

#include "logging.h"
#include "protocol_defs.h"
#include "spark_wiring_string.h"
#include "spark_wiring_timer.h"
#include "system_cloud.h"
#include "system_cloud_internal.h"
#include "system_publish_vitals.h"
#include "system_task.h"
#include "system_threading.h"
#include "system_update.h"
#include "system_cloud_internal.h"
#include "string_convert.h"
#include "spark_protocol_functions.h"
#include "events.h"
#include "deviceid_hal.h"
#include "system_mode.h"

#if PLATFORM_THREADING
#include "spark_wiring_timer.h"
#endif // PLATFORM_THREADING

extern void (*random_seed_from_cloud_handler)(unsigned int);

namespace
{

using namespace particle::system;

#if PLATFORM_THREADING
VitalsPublisher<Timer> _vitals;
#else  // not PLATFORM_THREADING
VitalsPublisher<particle::NullTimer> _vitals;
#endif // PLATFORM_THREADING

} // namespace

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

int spark_publish_vitals(system_tick_t period_s_, void* reserved_)
{
    SYSTEM_THREAD_CONTEXT_SYNC(spark_publish_vitals(period_s_, reserved_));
    int result;

    switch (period_s_)
    {
    case particle::NOW:
        result = _vitals.publish();
        break;
    case 0:
        _vitals.disablePeriodicPublish();
        result = _vitals.publish();
        break;
    default:
        _vitals.period(period_s_);
        _vitals.enablePeriodicPublish();
        result = _vitals.publish();
    }

    return spark_protocol_to_system_error(result);
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

void spark_unsubscribe(void *reserved)
{
    SYSTEM_THREAD_CONTEXT_ASYNC(spark_unsubscribe(reserved));
    spark_protocol_remove_event_handlers(sp, NULL);
}

static void spark_sync_time_impl()
{
    SYSTEM_THREAD_CONTEXT_ASYNC(spark_sync_time_impl());
    spark_protocol_send_time_request(sp);
}

bool spark_sync_time(void *reserved)
{
    spark_sync_time_impl();
    return spark_cloud_flag_connected();
}

bool spark_sync_time_pending(void* reserved)
{
    SYSTEM_THREAD_CONTEXT_SYNC(spark_sync_time_pending(reserved));
    return spark_protocol_time_request_pending(sp, nullptr);
}

system_tick_t spark_sync_time_last(time_t* tm, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_SYNC(spark_sync_time_last(tm, reserved));
    return spark_protocol_time_last_synced(sp, tm, nullptr);
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
    if (flags & PUBLISH_EVENT_FLAG_ASYNC) {
        SYSTEM_THREAD_CONTEXT_ASYNC_RESULT(spark_send_event(name, data, ttl, flags, reserved), true);
    }
    else {
    SYSTEM_THREAD_CONTEXT_SYNC(spark_send_event(name, data, ttl, flags, reserved));
    }

    spark_protocol_send_event_data d = { sizeof(spark_protocol_send_event_data) };
    if (reserved) {
        // Forward completion callback to the protocol implementation
        auto r = static_cast<const spark_send_event_data*>(reserved);
        d.handler_callback = r->handler_callback;
        d.handler_data = r->handler_data;
    }

    return spark_protocol_send_event(sp, name, data, ttl, convert(flags), &d);
}

bool spark_variable(const char *varKey, const void *userVar, Spark_Data_TypeDef userVarType, spark_variable_t* extra)
{
    SYSTEM_THREAD_CONTEXT_SYNC(spark_variable(varKey, userVar, userVarType, extra));

    User_Var_Lookup_Table_t* item = NULL;
    if (NULL != userVar && NULL != varKey && strlen(varKey)<=USER_VAR_KEY_LENGTH)
    {
    	item=find_var_by_key_or_add(varKey, userVar, userVarType, extra);
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

#endif // SPARK_NO_CLOUD

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
#endif // PLATFORM_THREADING

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

int spark_set_connection_property(unsigned property_id, unsigned data, particle::protocol::connection_properties_t* conn_prop, void* reserved)
{
    SYSTEM_THREAD_CONTEXT_SYNC(spark_set_connection_property(property_id, data, conn_prop, reserved));
    return spark_protocol_set_connection_property(sp, property_id, data, conn_prop, reserved);
}

int spark_set_random_seed_from_cloud_handler(void (*handler)(unsigned int), void* reserved)
{
#ifndef SPARK_NO_CLOUD
    random_seed_from_cloud_handler = handler;
#endif // SPARK_NO_CLOUD

    return 0;
}
