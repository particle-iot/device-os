/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "system_cloud_user_requests.h"

#include "system_task.h"
#include "system_threading.h"
#include "system_error.h"
#include "scope_guard.h"

#include <memory>
#include <cstdlib>
#include <cstring>

using particle::protocol::ProtocolError;

namespace {

struct GetUserVarTask {
    User_Var_Lookup_Table_t* item;
    SparkDescriptor::GetVariableCallback callback;
    void* context;
};

struct CallUserFunctionTask {
    User_Func_Lookup_Table_t* item;
    char* arg;
    SparkDescriptor::FunctionResultCallback callback;

    ~CallUserFunctionTask() {
        free(arg);
    }
};

SparkReturnType::Enum protocolVariableType(Spark_Data_TypeDef type) {
    switch (type) {
    case CLOUD_VAR_BOOLEAN:
        return SparkReturnType::BOOLEAN;
    case CLOUD_VAR_DOUBLE:
        return SparkReturnType::DOUBLE;
    case CLOUD_VAR_STRING:
        return SparkReturnType::STRING;
    default:
        return SparkReturnType::INT;
    }
}

size_t variableDataSize(const void* data, Spark_Data_TypeDef type) {
    switch (type) {
    case CLOUD_VAR_BOOLEAN:
        return sizeof(bool);
    case CLOUD_VAR_DOUBLE:
        return sizeof(double);
    case CLOUD_VAR_STRING:
        return data ? strlen((const char*)data) : 0;
    default:
        return sizeof(uint32_t);
    }
}

void getUserVarResult(int error, int type, void* data, size_t size, SparkDescriptor::GetVariableCallback callback,
        void* context) {
    SYSTEM_THREAD_CONTEXT_ASYNC(getUserVarResult(error, type, data, size, callback, context));
    callback(error, type, data, size, context);
}

// Called in the application thread
void getUserVarApp(void* arg) {
    std::unique_ptr<GetUserVarTask> task(static_cast<GetUserVarTask*>(arg));
    auto item = task->item;
    auto callback = task->callback;
    auto context = task->context;
    size_t size = 0;
    void* copy = nullptr;
    NAMED_SCOPE_GUARD(copyGuard, {
        free(copy);
    });
    if (item->copy) {
        const int result = item->copy(item->userVar, &copy, &size);
        if (result < 0) {
            getUserVarResult(ProtocolError::NO_MEMORY, 0 /* type */, nullptr /* data */, 0 /* size */, callback, context);
            return;
        }
    } else {
        const void* data = nullptr;
        if (item->update) {
            data = item->update(item->userVarKey, item->userVarType, item->userVar, nullptr);
        } else {
            data = item->userVar;
        }
        size = variableDataSize(data, item->userVarType);
        if (size > 0) {
            copy = malloc(size);
            if (!copy) {
                getUserVarResult(ProtocolError::NO_MEMORY, 0, nullptr, 0, callback, context);
                return;
            }
            memcpy(copy, data, size);
        }
    }
    const auto type = protocolVariableType(item->userVarType); // Spark_Data_TypeDef -> SparkReturnType::Enum
    getUserVarResult(ProtocolError::NO_ERROR, type, copy, size, callback, context);
    copyGuard.dismiss();
}

// Called in the application thread
void callUserFunctionApp(void* arg) {
    std::unique_ptr<CallUserFunctionTask> task(static_cast<CallUserFunctionTask*>(arg));
    int result = task->item->pUserFunc(task->item->pUserFuncData, task->arg, NULL);
    auto callback = std::move(task->callback);
    task.reset();
    // run the cloud return on the system thread again
    SYSTEM_THREAD_CONTEXT_ASYNC(callback((const void*)long(result), SparkReturnType::INT));
    callback((const void*)long(result), SparkReturnType::INT);
}

} // namespace

void getUserVarValue(User_Var_Lookup_Table_t* item, SparkDescriptor::GetVariableCallback callback, void* context) {
    std::unique_ptr<GetUserVarTask> task(new(std::nothrow) GetUserVarTask{ item, callback, context });
    if (!task || application_thread_invoke(getUserVarApp, task.get(), nullptr /* reserved */) != 0) {
        getUserVarResult(ProtocolError::NO_MEMORY, 0 /* type */, nullptr /* data */, 0 /* size */, callback, context);
        return;
    }
    task.release(); // Owned by getUserVarApp()
}

int callUserFunction(User_Func_Lookup_Table_t* item, const char* arg, SparkDescriptor::FunctionResultCallback callback) {
    // ensure we have a copy since the oriignal isn't guaranteed to be available once this function returns.
    std::unique_ptr<CallUserFunctionTask> task(new(std::nothrow) CallUserFunctionTask{ item, strdup(arg), std::move(callback) });
    if (!task || !task->arg) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    if (application_thread_invoke(callUserFunctionApp, task.get(), nullptr /* reserved */) != 0) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    task.release(); // Owned by callUserFunctionApp()
    return 0;
}
