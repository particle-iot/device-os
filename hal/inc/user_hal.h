/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#ifndef USER_HAL_H_
#define USER_HAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "module_info.h"

typedef struct hal_user_module_descriptor {
    module_info_t info;
    void* (*pre_init)(void);
    void (*init)(void);
    void (*loop)(void);
    void (*setup)(void);
} hal_user_module_descriptor;

int hal_user_module_get_descriptor(hal_user_module_descriptor* desc);

#ifdef __cplusplus
}
#endif

#endif  /* USER_HAL_H_ */
