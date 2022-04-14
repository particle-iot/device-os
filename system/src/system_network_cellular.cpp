/*
 * Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.
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

#include "spark_wiring_platform.h"

#if Wiring_Cellular && !HAL_PLATFORM_IFAPI

#include "system_network_cellular.h"

// This function handles USART3 global interrupt request
extern "C" void HAL_USART3_Handler(void) {
    HAL_USART3_Handler_Impl(nullptr); // Provided by cellular HAL
}

#endif // Wiring_Cellular != 0 && !HAL_PLATFORM_IFAPI
