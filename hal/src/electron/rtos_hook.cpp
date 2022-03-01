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

#include "FreeRTOS.h"
#include "task.h"
#include "service_debug.h"
#include "concurrent_hal.h"
#include <atomic>

namespace {

std::atomic_bool sRestoreIdleThreadPriority(false);

} // anonymous

extern "C" {

#ifdef DEBUG_BUILD
void vApplicationStackOverflowHook(TaskHandle_t, char*) {
    PANIC(StackOverflow, "Stack overflow detected");
}
#endif

void vApplicationTaskDeleteHook(void* pvTaskToDelete, volatile BaseType_t* pxPendYield) {
    (void)pvTaskToDelete;
    (void)pxPendYield;

    // NOTE: this hook is executed within a critical section

    sRestoreIdleThreadPriority = true;

    // Temporarily raise IDLE thread priority to (configMAX_PRIORITIES - 1) (maximum)
    // to give it some processing time to clean up the deleted task resources.
    vTaskPrioritySet(xTaskGetIdleTaskHandle(), configMAX_PRIORITIES - 1);

    // Immediately request the scheduler to yield to now higher priority IDLE thread
    *pxPendYield = pdTRUE;
}

void vApplicationIdleHook(void) {
    if (sRestoreIdleThreadPriority.exchange(false)) {
        // Restore IDLE thread priority back to the default one
        vTaskPrioritySet(nullptr, tskIDLE_PRIORITY);
    }
}

} // extern "C"
