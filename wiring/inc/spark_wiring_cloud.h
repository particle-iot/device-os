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

#include <chrono>

#include "spark_wiring_string.h"
#include "events.h"
#include "system_cloud.h"
#include "system_sleep.h"
#include "system_tick_hal.h"
#include "spark_protocol_functions.h"
#include "spark_wiring_system.h"
#include "spark_wiring_watchdog.h"
#include "spark_wiring_async.h"
#include "spark_wiring_flags.h"
#include "spark_wiring_global.h"
#include "interrupts_hal.h"
#include "system_mode.h"

#include <functional>
#include <type_traits>

#define PARTICLE_DEPRECATED_API_DEFAULT_PUBLISH_SCOPE \
        PARTICLE_DEPRECATED_API("Beginning with 0.8.0 release, Particle.publish() will require event scope to be specified explicitly.");

#define PARTICLE_DEPRECATED_API_DEFAULT_SUBSCRIBE_SCOPE \
        PARTICLE_DEPRECATED_API("Beginning with 0.8.0 release, Particle.subscribe() will require event scope to be specified explicitly.");

typedef std::function<user_function_int_str_t> user_std_function_int_str_t;
typedef std::function<void (const char*, const char*)> wiring_event_handler_t;

#ifndef __XSTRING
#define	__STRING(x)	#x		/* stringify without expanding x */
#define	__XSTRING(x)	__STRING(x)	/* expand x, then stringify */
#endif

struct PublishFlagType; // Tag type for Particle.publish() flags
typedef particle::Flags<PublishFlagType, uint8_t> PublishFlags;
typedef PublishFlags::FlagType PublishFlag;

const PublishFlag PUBLIC(PUBLISH_EVENT_FLAG_PUBLIC);
const PublishFlag PRIVATE(PUBLISH_EVENT_FLAG_PRIVATE);
const PublishFlag NO_ACK(PUBLISH_EVENT_FLAG_NO_ACK);
const PublishFlag WITH_ACK(PUBLISH_EVENT_FLAG_WITH_ACK);

// Test if the paramater a regular C "string" literal
template <typename T>
struct is_string_literal {
    static constexpr bool value = std::is_array<T>::value && std::is_same<typename std::remove_extent<T>::type, char>::value;
};

class CloudDisconnectOptions {
public:
    CloudDisconnectOptions();

    CloudDisconnectOptions& graceful(bool enabled);
    bool graceful() const;
    bool isGracefulSet() const;

    CloudDisconnectOptions& timeout(unsigned timeout);
    unsigned timeout() const;
    bool isTimeoutSet() const;

    spark_cloud_disconnect_options toSystemOptions() const;
    static CloudDisconnectOptions fromSystemOptions(const spark_cloud_disconnect_options* options);

private:
    enum OptionFlag {
        GRACEFUL = SPARK_CLOUD_DISCONNECT_FLAG_GRACEFUL,
        TIMEOUT = SPARK_CLOUD_DISCONNECT_FLAG_TIMEOUT
    };

    unsigned flags_; // TODO: Use std::optional (C++17)
    unsigned timeout_;
    bool graceful_;

    CloudDisconnectOptions(unsigned flags, unsigned timeout, bool graceful);
};

class CloudClass {
public:
    template <typename T, typename... ArgsT>
    static inline bool variable(const T &name, ArgsT&&... args)
    {
        static_assert(!is_string_literal<T>::value || sizeof(name) <= USER_VAR_KEY_LENGTH + 1,
            "\n\nIn Particle.variable, name must be " __XSTRING(USER_VAR_KEY_LENGTH) " characters or less\n\n");

        return _variable(name, std::forward<ArgsT>(args)...);
    }

    // Prevent unsupported functions from being registered as boolean variables
    template<typename T, std::enable_if_t<std::is_same<T, bool>::value, std::nullptr_t> = nullptr>
    static inline bool _variable(const char* varKey, const T& var)
    {
        return _variable(varKey, &var, BOOLEAN);
    }

    static inline bool _variable(const char* varKey, const int& var)
    {
        return _variable(varKey, &var, INT);
    }

#if PLATFORM_ID!=3 && PLATFORM_ID!=20
    // compiling with gcc this function duplicates the previous one.
    static inline bool _variable(const char* varKey, const int32_t& var)
    {
        return _variable(varKey, &var, INT);
    }
#endif

    static inline bool _variable(const char* varKey, const uint32_t& var)
    {
        return _variable(varKey, &var, INT);
    }

#if PLATFORM_ID!=3
    static bool _variable(const char* varKey, const float& var)
    __attribute__((error("Please change the variable from type `float` to `double` for use with Particle.variable().")));
#endif

    static inline bool _variable(const char* varKey, const double& var)
    {
        return _variable(varKey, &var, DOUBLE);
    }

    static inline bool _variable(const char* varKey, const String& var)
    {
        return _variable(varKey, &var, STRING);
    }

    static inline bool _variable(const char* varKey, const char* var)
    {
        return _variable(varKey, var, STRING);
    }

    template<std::size_t N>
    static inline bool _variable(const char* varKey, const char var[N])
    {
        return _variable(varKey, var, STRING);
    }

    template<std::size_t N>
    static inline bool _variable(const char* varKey, const unsigned char var[N])
    {
        return _variable(varKey, var, STRING);
    }

    static inline bool _variable(const char *varKey, const uint8_t* userVar, const CloudVariableTypeString& userVarType)
    {
        return _variable(varKey, (const char*)userVar, userVarType);
    }

    template<typename T> static inline bool _variable(const char *varKey, typename T::PointerType userVar, const T& userVarType)
    {
        return spark_variable(varKey, (const void*)userVar, T::TYPE_ID, NULL);
    }

    static inline bool _variable(const char *varKey, const int32_t* userVar, const CloudVariableTypeInt& userVarType)
    {
        return spark_variable(varKey, (const void*)userVar, CloudVariableTypeInt::TYPE_ID, NULL);
    }

    static inline bool _variable(const char *varKey, const uint32_t* userVar, const CloudVariableTypeInt& userVarType)
    {
        return spark_variable(varKey, (const void*)userVar, CloudVariableTypeInt::TYPE_ID, NULL);
    }

    // Return clear errors for common misuses of Particle.variable()
    template<typename T, std::size_t N>
    static inline bool _variable(const char *varKey, const T (*userVar)[N], const CloudVariableTypeString& userVarType)
    {
        static_assert(sizeof(T)==0, "\n\nUse Particle.variable(\"name\", myVar, STRING); without & in front of myVar\n\n");
        return false;
    }

    template<typename T>
    static inline bool _variable(const T *varKey, const String *userVar, const CloudVariableTypeString& userVarType)
    {
        spark_variable_t extra = {};
        extra.size = sizeof(extra);
        extra.update = update_string_variable;
        return spark_variable(varKey, userVar, CloudVariableTypeString::TYPE_ID, &extra);
    }

    template<typename T>
    static inline bool _variable(const T *varKey, const String &userVar, const CloudVariableTypeString& userVarType)
    {
        static_assert(sizeof(T)==0, "\n\nIn Particle.variable(\"name\", myVar, STRING); myVar must be declared as char myVar[] not String myVar\n\n");
        return false;
    }

    template <typename T, class ... Types>
    static inline bool function(const T &name, Types ... args)
    {
        static_assert(!is_string_literal<T>::value || sizeof(name) <= USER_FUNC_KEY_LENGTH + 1,
            "\n\nIn Particle.function, name must be " __XSTRING(USER_FUNC_KEY_LENGTH) " characters or less\n\n");

        return _function(name, args...);
    }

    static bool _function(const char *funcKey, user_function_int_str_t* func)
    {
        return register_function(call_raw_user_function, (void*)func, funcKey);
    }

    static bool _function(const char *funcKey, user_std_function_int_str_t func, void* reserved=NULL)
    {
        bool success = false;
        if (func) // if the call-wrapper has wrapped a callable object
        {
            auto wrapper = new user_std_function_int_str_t(func);
            if (wrapper) {
                success = register_function(call_std_user_function, wrapper, funcKey);
            }
        }
        return success;
    }

    template <typename T>
    static bool _function(const char *funcKey, int (T::*func)(String), T *instance) {
      using namespace std::placeholders;
      return _function(funcKey, std::bind(func, instance, _1));
    }

    inline particle::Future<bool> publish(const char *eventName, PublishFlags flags1, PublishFlags flags2 = PublishFlags())
    {
        return publish(eventName, NULL, flags1, flags2);
    }

    inline particle::Future<bool> publish(const char *eventName, const char *eventData, PublishFlags flags1, PublishFlags flags2 = PublishFlags())
    {
        return publish(eventName, eventData, DEFAULT_CLOUD_EVENT_TTL, flags1, flags2);
    }

    inline particle::Future<bool> publish(const char *eventName, const char *eventData, int ttl, PublishFlags flags1, PublishFlags flags2 = PublishFlags())
    {
        return publish_event(eventName, eventData, ttl, flags1 | flags2);
    }

    // Deprecated methods
    particle::Future<bool> publish(const char* name) PARTICLE_DEPRECATED_API_DEFAULT_PUBLISH_SCOPE;
    particle::Future<bool> publish(const char* name, const char* data) PARTICLE_DEPRECATED_API_DEFAULT_PUBLISH_SCOPE;
    particle::Future<bool> publish(const char* name, const char* data, int ttl) PARTICLE_DEPRECATED_API_DEFAULT_PUBLISH_SCOPE;

    /**
     * @brief Publish vitals information
     *
     * Provides a mechanism to control the interval at which system
     * diagnostic messages are sent to the cloud. Subsequently, this
     * controls the granularity of detail on the fleet health metrics.
     *
     * @param[in] period_s The period (in seconds) at which vitals messages are to be sent
     *                     to the cloud (default value: \p particle::NOW)
     * @arg \p particle::NOW - A special value used to send vitals immediately
     * @arg \p 0 - Publish a final message and disable periodic publishing
     * @arg \p s - Publish an initial message and subsequent messages every \p s seconds thereafter
     *
     * @returns \p system_error_t result code
     * @retval \p system_error_t::SYSTEM_ERROR_NONE
     * @retval \p system_error_t::SYSTEM_ERROR_IO
     *
     * @note At call time, a blocking call is made on the application thread. Any subsequent
     * timer-based calls are executed asynchronously.
     *
     * @note The periodic functionality is not available for the Spark Core.
     */
    int publishVitals(system_tick_t period_s = particle::NOW);
    inline int publishVitals(std::chrono::seconds s) { return publishVitals(s.count()); }

    inline bool subscribe(const char *eventName, EventHandler handler, Spark_Subscription_Scope_TypeDef scope)
    {
        return spark_subscribe(eventName, handler, NULL, scope, NULL, NULL);
    }

    inline bool subscribe(const char *eventName, EventHandler handler, const char *deviceID)
    {
        return spark_subscribe(eventName, handler, NULL, MY_DEVICES, deviceID, NULL);
    }

    bool subscribe(const char *eventName, wiring_event_handler_t handler, Spark_Subscription_Scope_TypeDef scope)
    {
        return subscribe_wiring(eventName, handler, scope);
    }

    bool subscribe(const char *eventName, wiring_event_handler_t handler, const char *deviceID)
    {
        return subscribe_wiring(eventName, handler, MY_DEVICES, deviceID);
    }

    template <typename T>
    bool subscribe(const char *eventName, void (T::*handler)(const char *, const char *), T *instance, Spark_Subscription_Scope_TypeDef scope)
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

    // Deprecated methods
    bool subscribe(const char* name, EventHandler handler) PARTICLE_DEPRECATED_API_DEFAULT_SUBSCRIBE_SCOPE;
    bool subscribe(const char* name, wiring_event_handler_t handler) PARTICLE_DEPRECATED_API_DEFAULT_SUBSCRIBE_SCOPE;
    template<typename T>
    bool subscribe(const char* name, void (T::*handler)(const char*, const char*), T* instance) PARTICLE_DEPRECATED_API_DEFAULT_SUBSCRIBE_SCOPE;

    void unsubscribe()
    {
        spark_unsubscribe(NULL);
    }

    bool syncTime(void)
    {
        if (!connected()) {
            return false;
        }
        return spark_sync_time(NULL);
    }

    bool syncTimePending(void)
    {
        return connected() && spark_sync_time_pending(nullptr);
    }

    bool syncTimeDone(void)
    {
        return !spark_sync_time_pending(nullptr) || disconnected();
    }

    system_tick_t timeSyncedLast(void)
    {
        time_t dummy;
        return timeSyncedLast(dummy);
    }

    system_tick_t timeSyncedLast(time_t& tm)
    {
        tm = 0;
        return spark_sync_time_last(&tm, nullptr);
    }

    static void sleep(long seconds) __attribute__ ((deprecated("Please use System.sleep() instead.")))
    { SystemClass::sleep(seconds); }
    static void sleep(Spark_Sleep_TypeDef sleepMode, long seconds=0) __attribute__ ((deprecated("Please use System.sleep() instead.")))
    { SystemClass::sleep(sleepMode, seconds); }
    static void sleep(uint16_t wakeUpPin, InterruptMode edgeTriggerMode, long seconds=0) __attribute__ ((deprecated("Please use System.sleep() instead.")))
    { SystemClass::sleep(wakeUpPin, edgeTriggerMode, seconds); }

    static bool connected(void) { return spark_cloud_flag_connected(); }
    static bool disconnected(void) { return !connected(); }
    static void connect(void) {
        spark_cloud_flag_connect();
    }
    static void disconnect(const CloudDisconnectOptions& options = CloudDisconnectOptions());
    static void process(void) {
    		application_checkin();
    		spark_process();
    }
    static String deviceID(void) { return SystemClass::deviceID(); }

#if HAL_PLATFORM_CLOUD_UDP
    inline static void keepAlive(unsigned sec)
    {
        particle::protocol::connection_properties_t conn_prop = {0};
        conn_prop.size = sizeof(conn_prop);
        conn_prop.keepalive_source = particle::protocol::KeepAliveSource::USER;
        spark_set_connection_property(SPARK_CLOUD_PING_INTERVAL, sec * 1000, &conn_prop, nullptr);
    }

    inline static void keepAlive(std::chrono::seconds s) { keepAlive(s.count()); }
#endif

    /**
     * Set the default cloud disconnection options.
     *
     * @param options Options.
     *
     * @see `disconnect()`
     */
    static void setDisconnectOptions(const CloudDisconnectOptions& options);

private:

    static bool register_function(cloud_function_t fn, void* data, const char* funcKey);
    static int call_raw_user_function(void* data, const char* param, void* reserved);
    static int call_std_user_function(void* data, const char* param, void* reserved);

    static void call_wiring_event_handler(const void* param, const char *event_name, const char *data);

    static particle::Future<bool> publish_event(const char *eventName, const char *eventData, int ttl, PublishFlags flags);

    static ProtocolFacade* sp()
    {
        return spark_protocol_instance();
    }

    bool subscribe_wiring(const char *eventName, wiring_event_handler_t handler, Spark_Subscription_Scope_TypeDef scope, const char *deviceID = NULL)
    {
        bool success = false;
        if (handler) // if the call-wrapper has wrapped a callable object
        {
            auto wrapper = new wiring_event_handler_t(handler);
            if (wrapper) {
                success = spark_subscribe(eventName, (EventHandler)call_wiring_event_handler, wrapper, scope, deviceID, NULL);
            }
        }
        return success;
    }

    static const void* update_string_variable(const char* name, Spark_Data_TypeDef type, const void* var, void* reserved)
    {
        const String* s = (const String*)var;
        return s->c_str();
    }

    // This method takes an argument of any callable type that requires no arguments and returns
    // a value of one of the supported variable types
    template<typename T, typename CloudVariableType<typename std::result_of<T()>::type>::ValueType* = nullptr>
    static bool _variable(const char *varKey, T&& fn)
    {
        return register_variable_fn(varKey, std::forward<T>(fn));
    }

    template<typename T>
    static int copy_variable_value(const T& val, void*& data, size_t& size) {
        size = sizeof(T);
        data = malloc(sizeof(T));
        if (!data) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        memcpy(data, &val, sizeof(T));
        return 0;
    }

    static int copy_variable_value(const char* str, void*& data, size_t& size) {
        size = str ? strlen(str) : 0;
        data = malloc(size);
        if (!data) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        memcpy(data, str, size);
        return 0;
    }

    static int copy_variable_value(const String& str, void*& data, size_t& size) {
        size = str.length();
        data = malloc(size);
        if (!data) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        memcpy(data, str.c_str(), size);
        return 0;
    }

    // Registers a function as a variable
    template<typename T, std::enable_if_t<std::is_function<T>::value, std::nullptr_t> = nullptr>
    static bool register_variable_fn(const char* varKey, const T& fn) {
        using VariableType = CloudVariableType<typename std::result_of<T&()>::type>;
        spark_variable_t extra = {};
        extra.size = sizeof(extra);
        extra.copy = [](const void* var, void** data, size_t* size) {
            const auto fn = (const T*)var;
            const typename VariableType::ValueType val = fn();
            return copy_variable_value(val, *data, *size);
        };
        return spark_variable(varKey, (const void*)&fn, VariableType::TYPE_ID, &extra);
    }

    // Registers a callable object, e.g. an std::function, as a variable
    template<typename T>
    static bool register_variable_fn(const char* varKey, T&& fn) {
        using CallableType = typename std::remove_reference<T>::type;
        using VariableType = CloudVariableType<typename std::result_of<T()>::type>;
        // Cloud variables cannot be unregistered so it's fine to allocate a copy of the callable on
        // the heap and never free it
        std::unique_ptr<CallableType> p(new(std::nothrow) CallableType(std::forward<T>(fn)));
        if (!p) {
            return false;
        }
        spark_variable_t extra = {};
        extra.size = sizeof(extra);
        extra.copy = [](const void* var, void** data, size_t* size) {
            const auto p = (CallableType*)var;
            const typename VariableType::ValueType val = (*p)();
            return copy_variable_value(val, *data, *size);
        };
        const bool ok = spark_variable(varKey, p.get(), VariableType::TYPE_ID, &extra);
        if (ok) {
            p.release();
        }
        return ok;
    }
};

extern CloudClass Spark __attribute__((deprecated("Spark is now Particle.")));
extern CloudClass Particle;

inline CloudDisconnectOptions::CloudDisconnectOptions() :
        CloudDisconnectOptions(0, 0, false) {
}

inline CloudDisconnectOptions::CloudDisconnectOptions(unsigned flags, unsigned timeout, bool graceful) :
        flags_(flags),
        timeout_(timeout),
        graceful_(graceful) {
}

inline CloudDisconnectOptions& CloudDisconnectOptions::graceful(bool enabled) {
    graceful_ = enabled;
    flags_ |= OptionFlag::GRACEFUL;
    return *this;
}

inline bool CloudDisconnectOptions::graceful() const {
    return graceful_;
}

inline bool CloudDisconnectOptions::isGracefulSet() const {
    return (flags_ & OptionFlag::GRACEFUL);
}

inline CloudDisconnectOptions& CloudDisconnectOptions::timeout(unsigned timeout) {
    timeout_ = timeout;
    flags_ |= OptionFlag::TIMEOUT;
    return *this;
}

inline unsigned CloudDisconnectOptions::timeout() const {
    return timeout_;
}

inline bool CloudDisconnectOptions::isTimeoutSet() const {
    return (flags_ & OptionFlag::TIMEOUT);
}

// Deprecated methods
inline particle::Future<bool> CloudClass::publish(const char* name) {
    return publish(name, PUBLIC);
}

inline particle::Future<bool> CloudClass::publish(const char* name, const char* data) {
    return publish(name, data, PUBLIC);
}

inline particle::Future<bool> CloudClass::publish(const char* name, const char* data, int ttl) {
    return publish(name, data, ttl, PUBLIC);
}

inline bool CloudClass::subscribe(const char* name, EventHandler handler) {
    return subscribe(name, handler, ALL_DEVICES);
}

inline bool CloudClass::subscribe(const char* name, wiring_event_handler_t handler) {
    return subscribe(name, handler, ALL_DEVICES);
}

template<typename T>
inline bool CloudClass::subscribe(const char* name, void (T::*handler)(const char*, const char*), T* instance) {
    return subscribe(name, handler, instance, ALL_DEVICES);
}
