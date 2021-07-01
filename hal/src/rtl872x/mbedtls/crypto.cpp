/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "mbedtls_util.h"
#include "timer_hal.h"

struct MbedTlsCallbackInitializer {
    MbedTlsCallbackInitializer() {
        mbedtls_callbacks_t cb = {0};
        cb.version = 0;
        cb.size = sizeof(cb);
        // cb.mbedtls_md_list = mbedtls_md_list;
        // cb.mbedtls_md_info_from_string = mbedtls_md_info_from_string;
        // cb.mbedtls_md_info_from_type = mbedtls_md_info_from_type;
        cb.millis = HAL_Timer_Get_Milli_Seconds;
        mbedtls_set_callbacks(&cb, NULL);
    }
};

MbedTlsCallbackInitializer s_mbedtls_callback_initializer;
