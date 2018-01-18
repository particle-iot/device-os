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

#define TRACER_SKIP_PLATFORM
#include "tracer_service.h"

#if defined(TRACER_USE_CALLBACKS)

static tracer_callbacks_t callbacks = {0};

int tracer_set_callbacks(tracer_callbacks_t* cb, void* reserved) {
    if (cb != nullptr) {
        callbacks = *cb;
    }
    return 0;
}

int tracer_save_checkpoint_(tracer_checkpoint_t* chkpt, uint32_t flags, void* reserved) {
    if (callbacks.tracer_save_checkpoint) {
        return callbacks.tracer_save_checkpoint(chkpt, flags, reserved);
    }

    return -1;
}

#else

int __attribute__((weak)) tracer_set_callbacks(tracer_callbacks_t* cb, void* reserved) {
    return -1;
}

#endif

#if defined(TRACER_USE_CALLBACKS2)

static tracer_callbacks_t callbacks2 = {0};

int tracer_set_callbacks_(tracer_callbacks_t* cb, void* reserved) {
    if (cb != nullptr) {
        callbacks2 = *cb;
    }
    return 0;
}

int tracer_save_checkpoint__(tracer_checkpoint_t* chkpt, uint32_t flags, void* reserved) {
    if (callbacks2.tracer_save_checkpoint) {
        return callbacks2.tracer_save_checkpoint(chkpt, flags, reserved);
    }

    return -1;
}

#else

int __attribute__((weak)) tracer_set_callbacks_(tracer_callbacks_t* cb, void* reserved) {
    return -1;
}

#endif
