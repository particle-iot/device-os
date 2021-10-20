/**
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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


#ifndef SYSTEM_CLOUD_INTERNAL_H
#define	SYSTEM_CLOUD_INTERNAL_H

#include "system_cloud.h"
#include "ota_flash_hal.h"
#include "socket_hal.h"
#include "spark_wiring_diagnostics.h"
#include "spark_wiring_cloud.h"
#include "atomic_flag_mutex.h"

void Spark_Signal(bool on, unsigned, void*);
void Spark_SetTime(unsigned long dateTime);
int Spark_Save(const void* buffer, size_t length, uint8_t type, void* reserved);
int Spark_Restore(void* buffer, size_t max_length, uint8_t type, void* reserved);

void Spark_Protocol_Init(void);
int Spark_Handshake(bool presence_announce);
bool Spark_Communication_Loop(void);
void Spark_Process_Events();

void system_set_time(uint32_t time, unsigned param, void* reserved);

String bytes2hex(const uint8_t* buf, unsigned len);

bool spark_function_internal(const cloud_function_descriptor* desc, void* reserved);
int call_raw_user_function(void* data, const char* param, void* reserved);

String spark_deviceID();

struct User_Var_Lookup_Table_t
{
    const void *userVar;
    Spark_Data_TypeDef userVarType;
    char userVarKey[USER_VAR_KEY_LENGTH+1];

    const void* (*update)(const char* name, Spark_Data_TypeDef varType, const void* var, void* reserved);
    int (*copy)(const void* var, void** data, size_t* size);
};


struct User_Func_Lookup_Table_t
{
    void* pUserFuncData;
    cloud_function_t pUserFunc;
    char userFuncKey[USER_FUNC_KEY_LENGTH+1];
};


User_Var_Lookup_Table_t* find_var_by_key_or_add(const char* varKey, const void* userVarData, Spark_Data_TypeDef userVarType, spark_variable_t* extra);
User_Func_Lookup_Table_t* find_func_by_key_or_add(const char* funcKey, const cloud_function_descriptor* desc);

extern ProtocolFacade* sp;

namespace particle {

class CloudDiagnostics {
public:
    // Note: Use odd numbers to encode transitional states
    enum Status {
        DISCONNECTED = 0,
        CONNECTING = 1,
        CONNECTED = 2,
        DISCONNECTING = 3
    };

    CloudDiagnostics() :
            status_(DIAG_ID_CLOUD_CONNECTION_STATUS, DIAG_NAME_CLOUD_CONNECTION_STATUS, DISCONNECTED),
            disconnReason_(DIAG_ID_CLOUD_DISCONNECTION_REASON, DIAG_NAME_CLOUD_DISCONNECTION_REASON, CLOUD_DISCONNECT_REASON_NONE),
            disconnCount_(DIAG_ID_CLOUD_DISCONNECTS, DIAG_NAME_CLOUD_DISCONNECTS),
            connCount_(DIAG_ID_CLOUD_CONNECTION_ATTEMPTS, DIAG_NAME_CLOUD_CONNECTION_ATTEMPTS),
            lastError_(DIAG_ID_CLOUD_CONNECTION_ERROR_CODE, DIAG_NAME_CLOUD_CONNECTION_ERROR_CODE) {
    }

    CloudDiagnostics& status(Status status) {
        status_ = status;
        return *this;
    }

    CloudDiagnostics& connectionAttempt() {
        ++connCount_;
        return *this;
    }

    CloudDiagnostics& resetConnectionAttempts() {
        connCount_ = 0;
        return *this;
    }

    CloudDiagnostics& disconnectionReason(cloud_disconnect_reason reason) {
        disconnReason_ = reason;
        return *this;
    }

    CloudDiagnostics& disconnectedUnexpectedly() {
        ++disconnCount_;
        return *this;
    }

    CloudDiagnostics& lastError(int error) {
        lastError_ = error;
        return *this;
    }

    static CloudDiagnostics* instance();

private:
    // Some of the diagnostic data sources use the synchronization since they can be updated from
    // the networking service thread
    AtomicEnumDiagnosticData<Status> status_;
    AtomicEnumDiagnosticData<cloud_disconnect_reason> disconnReason_;
    SimpleUnsignedIntegerDiagnosticData disconnCount_;
    SimpleUnsignedIntegerDiagnosticData connCount_;
    SimpleIntegerDiagnosticData lastError_;
};

class CloudConnectionSettings {
public:
    // Default disconnection settings
    static const bool DEFAULT_DISCONNECT_GRACEFULLY = false;
    static const unsigned DEFAULT_DISCONNECT_TIMEOUT = 30000;
    static const bool DEFAULT_DISCONNECT_CLEAR_SESSION = false;

    CloudConnectionSettings() :
            defaultDisconnectTimeout_(DEFAULT_DISCONNECT_TIMEOUT),
            defaultDisconnectGracefully_(DEFAULT_DISCONNECT_GRACEFULLY),
            defaultDisconnectClearSession_(DEFAULT_DISCONNECT_CLEAR_SESSION) {
    }

    void setDefaultDisconnectOptions(const CloudDisconnectOptions& options) {
        if (options.isGracefulSet()) {
            defaultDisconnectGracefully_ = options.graceful();
        }
        if (options.isTimeoutSet()) {
            defaultDisconnectTimeout_ = options.timeout();
        }
        if (options.isClearSessionSet()) {
            defaultDisconnectClearSession_ = options.clearSession();
        }
    }

    bool defaultDisconnectGracefully() const {
        return defaultDisconnectGracefully_;
    }

    void setPendingDisconnectOptions(CloudDisconnectOptions options) {
        const std::lock_guard<SimpleAtomicFlagMutex> lock(mutex_);
        pendingDisconnectOptions_ = std::move(options);
    }

    CloudDisconnectOptions takePendingDisconnectOptions() {
        CloudDisconnectOptions pending;
        {
            std::lock_guard<SimpleAtomicFlagMutex> lock(mutex_);
            using std::swap;
            swap(pending, pendingDisconnectOptions_);
        }
        CloudDisconnectOptions result;
        if (pending.isGracefulSet()) {
            result.graceful(pending.graceful());
        } else {
            result.graceful(defaultDisconnectGracefully_);
        }
        if (pending.isTimeoutSet()) {
            result.timeout(pending.timeout());
        } else {
            result.timeout(defaultDisconnectTimeout_);
        }
        if (pending.isClearSessionSet()) {
            result.clearSession(pending.clearSession());
        } else {
            result.clearSession(defaultDisconnectClearSession_);
        }
        return result;
    }

    static CloudConnectionSettings* instance();

private:
    SimpleAtomicFlagMutex mutex_;
    // Default disconnection options can only be set in the context of the system thread.
    // defaultDisconnectGracefully_ may also be accessed from an ISR
    unsigned defaultDisconnectTimeout_;
    volatile bool defaultDisconnectGracefully_;
    bool defaultDisconnectClearSession_;
    // Pending disconnection options are set atomically and guarded by a spinlock
    CloudDisconnectOptions pendingDisconnectOptions_;
};

// Use this function instead of Particle.publish() in the system code
inline bool publishEvent(const char* event, const char* data = nullptr, unsigned flags = 0) {
    return spark_send_event(event, data, DEFAULT_CLOUD_EVENT_TTL, flags | PUBLISH_EVENT_FLAG_PRIVATE, nullptr);
}

// Sends application DESCRIBE and subscriptions
int sendApplicationDescription();

// Subscribes to system cloud events
void registerSystemSubscriptions();

// Invalidates the cached session data
void clearSessionData();

size_t cloudVariableCount();
int getCloudVariableInfo(size_t index, const char** name, int* type = nullptr);
size_t cloudFunctionCount();
int getCloudFunctionInfo(size_t index, const char** name);

} // namespace particle

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

size_t system_interpolate_cloud_server_hostname(const char* var, size_t var_len, char* buf, size_t buf_len);

void invokeEventHandler(uint16_t handlerInfoSize, FilteringEventHandler* handlerInfo,
                const char* event_name, const char* event_data, void* reserved);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* SYSTEM_CLOUD_INTERNAL_H */
