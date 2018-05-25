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

#pragma once

#include "service_debug.h"

#include <FreeRTOS.h>
#include <semphr.h>

/* TODO: consolidate with stm32f2x version */
class StaticRecursiveMutex {
public:
    StaticRecursiveMutex() {
    }

    ~StaticRecursiveMutex() {
        vSemaphoreDelete(mutex());
    }

    bool lock(unsigned timeout = 0) {
        const TickType_t t = (timeout > 0) ? (timeout / portTICK_PERIOD_MS) : portMAX_DELAY;
        return (xSemaphoreTakeRecursive(mutex(), t) == pdTRUE);
    }

    bool unlock() {
        return (xSemaphoreGiveRecursive(mutex()) == pdTRUE);
    }

private:
    StaticSemaphore_t mutexBuffer_;

    SemaphoreHandle_t mutex() {
        static SemaphoreHandle_t m = xSemaphoreCreateRecursiveMutexStatic(&mutexBuffer_);
        return m;
    }
};
