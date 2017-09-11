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

#define DIAGNOSTIC_SKIP_PLATFORM
#include "diagnostic.h"

#if defined(DIAGNOSTIC_USE_CALLBACKS)

static diagnostic_callbacks_t callbacks = {0};

int diagnostic_set_callbacks(diagnostic_callbacks_t* cb, void* reserved) {
    if (cb != nullptr) {
        callbacks = *cb;
    }
    return 0;
}

int diagnostic_save_checkpoint_(diagnostic_checkpoint_t* chkpt, uint32_t flags, void* reserved) {
    if (callbacks.diagnostic_save_checkpoint) {
        return callbacks.diagnostic_save_checkpoint(chkpt, flags, reserved);
    }

    return -1;
}

#else

int __attribute__((weak)) diagnostic_set_callbacks(diagnostic_callbacks_t* cb, void* reserved) {
    return -1;
}

#endif

#if defined(DIAGNOSTIC_USE_CALLBACKS2)

static diagnostic_callbacks_t callbacks2 = {0};

int diagnostic_set_callbacks_(diagnostic_callbacks_t* cb, void* reserved) {
    if (cb != nullptr) {
        callbacks2 = *cb;
    }
    return 0;
}

int diagnostic_save_checkpoint__(diagnostic_checkpoint_t* chkpt, uint32_t flags, void* reserved) {
    if (callbacks2.diagnostic_save_checkpoint) {
        return callbacks2.diagnostic_save_checkpoint(chkpt, flags, reserved);
    }

    return -1;
}

#else

int __attribute__((weak)) diagnostic_set_callbacks_(diagnostic_callbacks_t* cb, void* reserved) {
    return -1;
}

#endif
