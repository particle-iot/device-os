/**
 ******************************************************************************
 * @file    spark_wiring_cloud.h
 * @author  Satish Nair, Zachary Crockett, Matthew McGowan
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

#include "spark_wiring_string.h"
#include "events.h"
#include "system_cloud.h"
#include "spark_protocol_functions.h"
#include "spark_wiring_system.h"
#include <functional>

typedef std::function<user_function_int_str_t> user_std_function_int_str_t;

#ifdef SPARK_NO_CLOUD
#define CLOUD_FN(x,y) (y)
#else
#define CLOUD_FN(x,y) (x)
#endif

class CloudClass {
    
        
public:
    static bool variable(const char *varKey, const void *userVar, Spark_Data_TypeDef userVarType) 
    {
        return CLOUD_FN(spark_variable(varKey, userVar, userVarType, NULL), false);
    }

    static bool function(const char *funcKey, user_function_int_str_t* func)
    {
        return CLOUD_FN(register_function(call_raw_user_function, (void*)func, funcKey), false);        
    }
    
    static bool function(const char *funcKey, user_std_function_int_str_t func, void* reserved=NULL)
    {
#ifdef SPARK_NO_CLOUD
        return false;
#else
        bool success = false;
        if (func) // if the call-wrapper has wrapped a callable object
        {            
            auto wrapper = new user_std_function_int_str_t(func);
            if (wrapper) {
                success = register_function(call_std_user_function, wrapper, funcKey);                    
            }
        }
        return success;       
#endif        
    }

    bool publish(const char *eventName, Spark_Event_TypeDef eventType=PUBLIC)
    {
        return CLOUD_FN(spark_send_event(eventName, NULL, 60, eventType, NULL), false);
    }

    bool publish(const char *eventName, const char *eventData, Spark_Event_TypeDef eventType=PUBLIC)
    {
        return CLOUD_FN(spark_send_event(eventName, eventData, 60, eventType, NULL), false);
    }

    bool publish(const char *eventName, const char *eventData, int ttl, Spark_Event_TypeDef eventType=PUBLIC)
    {
        return CLOUD_FN(spark_send_event(eventName, eventData, ttl, eventType, NULL), false);
    }

    bool subscribe(const char *eventName, EventHandler handler, Spark_Subscription_Scope_TypeDef scope=ALL_DEVICES)
    {
        return CLOUD_FN(spark_subscribe(eventName, handler, NULL, scope, NULL, NULL), false);
    }

    bool subscribe(const char *eventName, EventHandler handler, const char *deviceID)
    {
        return CLOUD_FN(spark_subscribe(eventName, handler, NULL, MY_DEVICES, deviceID, NULL), false);
    }
    
    void unsubscribe() 
    {
        CLOUD_FN(spark_protocol_remove_event_handlers(sp(), NULL), (void)0);
    }

    void syncTime(void)
    {
        CLOUD_FN(spark_protocol_send_time_request(sp()),(void)0);
    }
    
    static void sleep(long seconds) __attribute__ ((deprecated("Please use System.sleep() instead.")))
    { SystemClass::sleep(seconds); }    
    static void sleep(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds=0) __attribute__ ((deprecated("Please use System.sleep() instead.")))
    { SystemClass::sleep(wakeUpPin, edgeTriggerMode, seconds); }
    
    static bool connected(void) { return spark_connected(); }
    static void connect(void) { spark_connect(); }
    static void disconnect(void) { spark_disconnect(); }
    static void process(void) { spark_process(); }
    static String deviceID(void) { return SystemClass::deviceID(); }
    
private:

    static bool register_function(cloud_function_t fn, void* data, const char* funcKey);
    static int call_raw_user_function(void* data, const char* param, void* reserved);
    static int call_std_user_function(void* data, const char* param, void* reserved);
    
    SparkProtocol* sp() { return spark_protocol_instance(); }
};


extern CloudClass Spark;
extern CloudClass Cloud;
