/**
 ******************************************************************************
 * @file    spark_wiring_cloud.h
 * @author  Satish Nair, Zachary Crockett, Matthew McGowan
 ******************************************************************************
  Copyright (c) 2013-2015 Spark Labs, Inc.  All rights reserved.

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

typedef enum
{
  MY_DEVICES
} Spark_Subscription_Scope_TypeDef;


class SparkClass {
    
    
    inline static EventType::Enum convert(Spark_Event_TypeDef eventType) {
        return eventType==PUBLIC ? EventType::PUBLIC : EventType::PRIVATE;
    }
    
public:
    static void variable(const char *varKey, void *userVar, Spark_Data_TypeDef userVarType) 
    {
        spark_variable(varKey, userVar, userVarType, NULL);
    }
    static void function(const char *funcKey, int (*pFunc)(String paramString)) 
    {
        spark_function(funcKey, pFunc, NULL);
    }

    bool publish(const char *eventName, Spark_Event_TypeDef eventType=PUBLIC)
    {
        return spark_protocol_send_event(sp(), eventName, NULL, 60, convert(eventType));
    }

    bool publish(const char *eventName, const char *eventData, Spark_Event_TypeDef eventType=PUBLIC)
    {
        return spark_protocol_send_event(sp(), eventName, eventData, 60, convert(eventType));
    }

    bool publish(const char *eventName, const char *eventData, int ttl, Spark_Event_TypeDef eventType=PUBLIC)
    {
        return spark_protocol_send_event(sp(), eventName, eventData, ttl, convert(eventType));
    }

    bool publish(String eventName, Spark_Event_TypeDef eventType=PUBLIC)
    {
        return publish(eventName.c_str(), eventType);
    }

    bool publish(String eventName, String eventData, Spark_Event_TypeDef eventType=PUBLIC)
    {
        return publish(eventName.c_str(), eventData.c_str(), eventType);
    }

    bool publish(String eventName, String eventData, int ttl, Spark_Event_TypeDef eventType=PUBLIC)
    {
        return publish(eventName.c_str(), eventData.c_str(), ttl, eventType);
    }

    bool subscribe(const char *eventName, EventHandler handler)
    {
        bool success = spark_protocol_add_event_handler(sp(), eventName, handler, SubscriptionScope::FIREHOSE, NULL);
        if (success && connected())
        {
            success = spark_protocol_send_subscription_scope(sp(), eventName, SubscriptionScope::FIREHOSE);
        }
        return success;
    }

    bool subscribe(const char *eventName, EventHandler handler, Spark_Subscription_Scope_TypeDef scope)
    {
        bool success = spark_protocol_add_event_handler(sp(), eventName, handler, SubscriptionScope::MY_DEVICES, NULL);
        if (success && connected())
        {
            success = spark_protocol_send_subscription_scope(sp(), eventName, SubscriptionScope::MY_DEVICES);
        }
        return success;
    }

    bool subscribe(const char *eventName, EventHandler handler, const char *deviceID)
    {
        bool success = spark_protocol_add_event_handler(sp(), eventName, handler, SubscriptionScope::MY_DEVICES, deviceID);
        if (success && connected())
        {
            success = spark_protocol_send_subscription_device(sp(), eventName, deviceID);
        }
        return success;
    }

    bool subscribe(String eventName, EventHandler handler)
    {
        return subscribe(eventName.c_str(), handler);
    }

    bool subscribe(String eventName, EventHandler handler, Spark_Subscription_Scope_TypeDef scope)
    {
        return subscribe(eventName.c_str(), handler, scope);
    }

    bool subscribe(String eventName, EventHandler handler, String deviceID)
    {
        return subscribe(eventName.c_str(), handler, deviceID.c_str());
    }
    
    void unsubscribe() 
    {
        spark_protocol_remove_event_handlers(sp(), NULL);
    }

    void syncTime(void)
    {
        spark_protocol_send_time_request(sp());
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

    SparkProtocol* sp() { return spark_protocol_instance(); }
};


extern SparkClass Spark;
