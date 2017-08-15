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
#include "diagnostic.h"

#if defined(PLATFORM_DIAGNOSTIC_ENABLED)
#include "diagnostic_impl.h"

using namespace particle::diagnostic;

struct DiagnosticServiceInitializer {
    DiagnosticServiceInitializer() {
        auto s = DiagnosticService::lock(true);

        auto diag = DiagnosticService::instance();
        (void)diag;

        diagnostic_callbacks_t cb = {0};
        cb.diagnostic_save_checkpoint = &diagnostic_save_checkpoint;
        diagnostic_set_callbacks(&cb, nullptr);
#if defined(MODULE_HAS_SYSTEM_PART3) && MODULE_HAS_SYSTEM_PART3 == 1
        diagnostic_set_callbacks_(&cb, nullptr);
#endif /* defined(MODULE_HAS_SYSTEM_PART3) && MODULE_HAS_SYSTEM_PART3 == 1 */

        DiagnosticService::unlock(true, s);
    }
};

static DiagnosticServiceInitializer s_initializer;

struct diag_save_data_t {
    DiagnosticService* diag;
    diagnostic_checkpoint_t* chkpt;
    os_unique_id_t id;
    uint32_t flags;
    diagnostic_error_t err;
};

int diagnostic_save_checkpoint(diagnostic_checkpoint_t* chkpt, uint32_t flags, void* reserved)
{
    diagnostic_error_t err = DIAGNOSTIC_ERROR_NONE;
    os_thread_t thread = os_thread_current();
    os_unique_id_t id = os_thread_unique_id(thread);

    // bool isr = HAL_IsISR();
    bool isr = true;

    uintptr_t st = DiagnosticService::lock(isr);
    auto diag = DiagnosticService::instance();

    if (diag->frozen()) {
        DiagnosticService::unlock(isr, st);
        return 1;
    }
    if (flags & DIAGNOSTIC_FLAG_DUMP_STACKTRACES) {
        diag->cleanStacktraces();
        diag->unmarkAll();

        diag_save_data_t d = {diag, chkpt, id, flags, DIAGNOSTIC_ERROR};
        os_thread_dump(OS_THREAD_INVALID_HANDLE, [](os_thread_dump_info_t* info, void* data) -> os_result_t {
            diag_save_data_t* d = (diag_save_data_t*)data;
            d->err = d->diag->insertCheckpoint(info, info->id == d->id ? d->chkpt : nullptr, true, true);
            return 0;
        }, &d);
        diag->removeUnmarked();
        err = d.err;
    } else if (chkpt) {
        err = diag->insertCheckpoint(id, chkpt);
        if (err == DIAGNOSTIC_ERROR_NO_THREAD_ENTRY) {
            diag_save_data_t d = {diag, chkpt, id, flags, DIAGNOSTIC_ERROR};
            os_thread_dump(thread, [](os_thread_dump_info_t* info, void* data) -> os_result_t {
                diag_save_data_t* d = (diag_save_data_t*)data;
                d->err = d->diag->insertCheckpoint(info, d->chkpt, false);
                return 1;
            }, &d);
            err = d.err;
        }
    }
    diag->updateCrc();
    if (flags & DIAGNOSTIC_FLAG_FREEZE) {
        diag->freeze(true);
    }
    DiagnosticService::unlock(isr, st);

    return 0;
}

size_t diagnostic_dump_current(char* buf, size_t bufSize) {
    // bool isr = HAL_IsISR();
    bool isr = true;
    uintptr_t st = DiagnosticService::lock(isr);
    auto diag = DiagnosticService::instance();
    size_t sz = diag->dumpCurrent(buf, bufSize);
    DiagnosticService::unlock(isr, st);

    return sz;
}

size_t diagnostic_dump_saved(char* buf, size_t bufSize) {
    auto diag = DiagnosticService::instance();
    size_t sz = diag->dumpSaved(buf, bufSize);

    return sz;
}

#endif /* PLATFORM_DIAGNOSTIC_ENABLED */
