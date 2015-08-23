/**
  ******************************************************************************
  * @file    particle_descriptor.h
  * @authors  Zachary Crockett
  * @version V1.0.0
  * @date    15-Nov-2013
  * @brief   SPARK DESCRIPTOR
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

#include "appender.h"
#include "static_assert.h"
#include <functional>

// Deferring to ASN.1 type codes
namespace ParticleReturnType {
  enum Enum {
    BOOLEAN = 1,
    INT     = 2,
    STRING  = 4,
    DOUBLE  = 9
  };
}

struct ParticleDescriptor
{
    typedef std::function<bool(const void*, ParticleReturnType::Enum)> FunctionResultCallback;

    size_t size;
    int (*num_functions)(void);
    const char* (*get_function_key)(int function_index);
    int (*call_function)(const char *function_key, const char *arg, FunctionResultCallback callback, void* reserved);

    int (*num_variables)(void);
    const char* (*get_variable_key)(int variable_index);
    ParticleReturnType::Enum (*variable_type)(const char *variable_key);
    const void *(*get_variable)(const char *variable_key);

    bool (*was_ota_upgrade_successful)(void);
    void (*ota_upgrade_status_sent)(void);

    bool (*append_system_info)(appender_fn appender, void* append, void* reserved);

    void* reserved[4];      // add a few additional pointers
};

STATIC_ASSERT(ParticleDescriptor_size, sizeof(ParticleDescriptor)==60 || sizeof(void*)!=4);
