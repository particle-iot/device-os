/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
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

#include "concurrent_hal.h"
#include "core_hal.h"
#include <stdio.h>
#include "tracer_service.h"

#if defined(PLATFORM_TRACER_ENABLED)
#include "system_tracer_service_impl.h"

using namespace particle::tracer;

struct TracerServiceInitializer {
    TracerServiceInitializer() {
        auto s = TracerService::lock(true);

        auto tracer = TracerService::instance();
        (void)tracer;

        tracer_callbacks_t cb = {0};
        cb.tracer_save_checkpoint = &tracer_save_checkpoint;
        tracer_set_callbacks(&cb, nullptr);
#if defined(MODULE_HAS_SYSTEM_PART3) && MODULE_HAS_SYSTEM_PART3 == 1
        tracer_set_callbacks_(&cb, nullptr);
#endif /* defined(MODULE_HAS_SYSTEM_PART3) && MODULE_HAS_SYSTEM_PART3 == 1 */

        TracerService::unlock(true, s);
    }
};

static TracerServiceInitializer s_initializer;

struct tracer_save_data_t {
    TracerService* tracer;
    tracer_checkpoint_t* chkpt;
    os_unique_id_t id;
    uint32_t flags;
    tracer_error_t err;
};

int tracer_save_checkpoint(tracer_checkpoint_t* chkpt, uint32_t flags, void* reserved)
{
    tracer_error_t err = TRACER_ERROR_NONE;
    os_thread_t thread = os_thread_current();
    os_unique_id_t id = os_thread_unique_id(thread);

    // bool isr = HAL_IsISR();
    bool isr = true;

    uintptr_t st = TracerService::lock(isr);
    auto tracer = TracerService::instance();

    if (tracer->frozen()) {
        TracerService::unlock(isr, st);
        return 1;
    }
    if (flags & TRACER_FLAG_DUMP_STACKTRACES) {
        tracer->cleanStacktraces();
        tracer->unmarkAll();

        tracer_save_data_t d = {tracer, chkpt, id, flags, TRACER_ERROR};
        os_thread_dump(OS_THREAD_INVALID_HANDLE, [](os_thread_dump_info_t* info, void* data) -> os_result_t {
            tracer_save_data_t* d = (tracer_save_data_t*)data;
            d->err = d->tracer->insertCheckpoint(info, info->id == d->id ? d->chkpt : nullptr, true, true);
            return 0;
        }, &d);
        tracer->removeUnmarked();
        err = d.err;
    } else if (chkpt) {
        err = tracer->insertCheckpoint(id, chkpt);
        if (err == TRACER_ERROR_NO_THREAD_ENTRY) {
            tracer_save_data_t d = {tracer, chkpt, id, flags, TRACER_ERROR};
            os_thread_dump(thread, [](os_thread_dump_info_t* info, void* data) -> os_result_t {
                tracer_save_data_t* d = (tracer_save_data_t*)data;
                d->err = d->tracer->insertCheckpoint(info, d->chkpt, false);
                return 1;
            }, &d);
            err = d.err;
        }
    }
    tracer->updateCrc();
    if (flags & TRACER_FLAG_FREEZE) {
        tracer->freeze(true);
    }
    TracerService::unlock(isr, st);

    return 0;
}

size_t tracer_dump_current(char* buf, size_t bufSize) {
    // bool isr = HAL_IsISR();
    bool isr = true;
    uintptr_t st = TracerService::lock(isr);
    auto tracer = TracerService::instance();
    size_t sz = tracer->dumpCurrent(buf, bufSize);
    TracerService::unlock(isr, st);

    return sz;
}

size_t tracer_dump_saved(char* buf, size_t bufSize) {
    auto tracer = TracerService::instance();
    size_t sz = tracer->dumpSaved(buf, bufSize);

    return sz;
}

#endif /* PLATFORM_TRACER_ENABLED */
