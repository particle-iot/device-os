/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include "newlib_impure.h"

#if HAL_PLATFORM_NEWLIB

#include "concurrent_hal.h"

namespace {

newlib_impure_cb s_impure_cb = nullptr;
void* s_impure_cb_ctx = nullptr;

} // anonymous

void newlib_impure_ptr_callback(newlib_impure_cb cb, void* ctx) {
    os_thread_scheduling(false, nullptr);
    s_impure_cb = cb;
    s_impure_cb_ctx = ctx;
    os_thread_scheduling(true, nullptr);
    if (cb) {
        cb(_impure_ptr, sizeof(struct _reent), NEWLIB_VERSION_NUM, ctx);
    }
}

void newlib_impure_ptr_change(struct _reent* r) {
    if (s_impure_cb) {
        s_impure_cb(r, sizeof(struct _reent), NEWLIB_VERSION_NUM, s_impure_cb_ctx);
    }
    newlib_impure_ptr_change_module(r, sizeof(struct _reent), NEWLIB_VERSION_NUM);
}

__attribute__((weak)) void newlib_impure_ptr_change_module(struct _reent* r, size_t size, uint32_t version) {
    // Default implementation
}

#endif // HAL_PLATFORM_NEWLIB
