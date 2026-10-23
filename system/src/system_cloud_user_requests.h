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

#pragma once

#include "system_cloud_internal.h"
#include "spark_descriptor.h"

/**
 * Get the value of a cloud variable in the application thread.
 *
 * The callback is invoked in the system thread with the variable value or an error.
 */
void getUserVarValue(User_Var_Lookup_Table_t* item, SparkDescriptor::GetVariableCallback callback, void* context);

/**
 * Call a cloud function in the application thread.
 *
 * The callback is invoked in the system thread with the result of the function. It's not invoked if
 * this function returns an error.
 */
int callUserFunction(User_Func_Lookup_Table_t* item, const char* arg, SparkDescriptor::FunctionResultCallback callback);
