/**
 ******************************************************************************
 * @file    particle_wiring_cloud.h
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

#include "particle_wiring_string.h"
#include "events.h"
#include "system_cloud.h"
#include "system_sleep.h"
#include "particle_protocol_functions.h"
#include "particle_wiring_system.h"
#include "interrupts_hal.h"
#include <functional>

typedef std::function<user_function_int_str_t> user_std_function_int_str_t;
typedef std::function<void (const char*, const char*)> wiring_event_handler_t;

#ifdef PARTICLE_NO_CLOUD
#define CLOUD_FN(x,y) (y)
#else
#define CLOUD_FN(x,y) (x)
#endif

class CloudClass {


public:
    static bool variable(const char *varKey, const void *userVar, Particle_Data_TypeDef userVarType)
    {
        return CLOUD_FN(particle_variable(varKey, userVar, userVarType, NULL), false);
    }

    static bool function(const char *funcKey, user_function_int_str_t* func)
    {
        return CLOUD_FN(register_function(call_raw_user_function, (void*)func, funcKey), false);
    }

    static bool function(const char *funcKey, user_std_function_int_str_t func, void* reserved=NULL)
    {
#ifdef PARTICLE_NO_CLOUD
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

    template <typename T>
    static void function(const char *funcKey, int (T::*func)(String), T *instance) {
      using namespace std::placeholders;
      function(funcKey, std::bind(func, instance, _1));
    }

    bool publish(const char *eventName, Particle_Event_TypeDef eventType=PUBLIC)
    {
        return CLOUD_FN(particle_send_event(eventName, NULL, 60, eventType, NULL), false);
    }

    bool publish(const char *eventName, const char *eventData, Particle_Event_TypeDef eventType=PUBLIC)
    {
        return CLOUD_FN(particle_send_event(eventName, eventData, 60, eventType, NULL), false);
    }

    bool publish(const char *eventName, const char *eventData, int ttl, Particle_Event_TypeDef eventType=PUBLIC)
    {
        return CLOUD_FN(particle_send_event(eventName, eventData, ttl, eventType, NULL), false);
    }

    bool subscribe(const char *eventName, EventHandler handler, Particle_Subscription_Scope_TypeDef scope=ALL_DEVICES)
    {
        return CLOUD_FN(particle_subscribe(eventName, handler, NULL, scope, NULL, NULL), false);
    }

    bool subscribe(const char *eventName, EventHandler handler, const char *deviceID)
    {
        return CLOUD_FN(particle_subscribe(eventName, handler, NULL, MY_DEVICES, deviceID, NULL), false);
    }

    bool subscribe(const char *eventName, wiring_event_handler_t handler, Particle_Subscription_Scope_TypeDef scope=ALL_DEVICES)
    {
        return subscribe_wiring(eventName, handler, scope);
    }

    bool subscribe(const char *eventName, wiring_event_handler_t handler, const char *deviceID)
    {
        return subscribe_wiring(eventName, handler, MY_DEVICES, deviceID);
    }

    template <typename T>
    bool subscribe(const char *eventName, void (T::*handler)(const char *, const char *), T *instance, Particle_Subscription_Scope_TypeDef scope=ALL_DEVICES)
    {
        using namespace std::placeholders;
        return subscribe(eventName, std::bind(handler, instance, _1, _2), scope);
    }

    template <typename T>
    bool subscribe(const char *eventName, void (T::*handler)(const char *, const char *), T *instance, const char *deviceID)
    {
        using namespace std::placeholders;
        return subscribe(eventName, std::bind(handler, instance, _1, _2), deviceID);
    }

    void unsubscribe()
    {
        CLOUD_FN(particle_protocol_remove_event_handlers(pp(), NULL), (void)0);
    }

    bool syncTime(void)
    {
        return CLOUD_FN(particle_protocol_send_time_request(pp()),false);
    }

    static void sleep(long seconds) __attribute__ ((deprecated("Please use System.sleep() instead.")))
    { SystemClass::sleep(seconds); }
    static void sleep(Particle_Sleep_TypeDef sleepMode, long seconds=0) __attribute__ ((deprecated("Please use System.sleep() instead.")))
    { SystemClass::sleep(sleepMode, seconds); }
    static void sleep(uint16_t wakeUpPin, InterruptMode edgeTriggerMode, long seconds=0) __attribute__ ((deprecated("Please use System.sleep() instead.")))
    { SystemClass::sleep(wakeUpPin, edgeTriggerMode, seconds); }

    static bool connected(void) { return particle_connected(); }
    static void connect(void) { particle_connect(); }
    static void disconnect(void) { particle_disconnect(); }
    static void process(void) { particle_process(); }
    static String deviceID(void) { return SystemClass::deviceID(); }

private:

    static bool register_function(cloud_function_t fn, void* data, const char* funcKey);
    static int call_raw_user_function(void* data, const char* param, void* reserved);
    static int call_std_user_function(void* data, const char* param, void* reserved);

    static void call_wiring_event_handler(const void* param, const char *event_name, const char *data);

    ParticleProtocol* pp() { return particle_protocol_instance(); }

    bool subscribe_wiring(const char *eventName, wiring_event_handler_t handler, Particle_Subscription_Scope_TypeDef scope, const char *deviceID = NULL)
    {
#ifdef PARTICLE_NO_CLOUD
        return false;
#else
        bool success = false;
        if (handler) // if the call-wrapper has wrapped a callable object
        {
            auto wrapper = new wiring_event_handler_t(handler);
            if (wrapper) {
                success = particle_subscribe(eventName, (EventHandler)call_wiring_event_handler, wrapper, scope, deviceID, NULL);
            }
        }
        return success;
#endif
    }
};


extern CloudClass Spark __attribute__((deprecated("Spark is now Particle.")));
extern CloudClass Particle;
