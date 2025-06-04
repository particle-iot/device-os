/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

#include "runnable.h"
#include "concurrent_hal.h"
#include "FreeRTOS.h"
#include "queue.h"

namespace particle {

class FreeRunnable: public Runnable {
public:
    FreeRunnable();
    virtual ~FreeRunnable();

    virtual int run();

    int enqueue(void* ptr);
    int init(size_t queueSize, system_tick_t wait = CONCURRENT_WAIT_FOREVER);
private:
    os_queue_t queue_;
    system_tick_t wait_;
};

FreeRunnable::FreeRunnable()
        : Runnable(),
          queue_(nullptr),
          wait_(CONCURRENT_WAIT_FOREVER) {
}

FreeRunnable::~FreeRunnable() {
    if (queue_) {
        os_queue_destroy(queue_, nullptr);
    }
}

int FreeRunnable::run() {
    if (!queue_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    void* ptr = nullptr;
    os_queue_take(queue_, (void*)&ptr, wait_, nullptr);
    if (ptr) {
        free(ptr);
    }
    return 0;
}

int FreeRunnable::init(size_t queueSize, system_tick_t wait) {
    os_queue_create(&queue_, sizeof(uintptr_t), queueSize, nullptr);
    if (!queue_) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    wait_ = wait;
    return 0;
}

int FreeRunnable::enqueue(void* ptr) {
    if (!ptr) {
        return 0;
    }

    if (!queue_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    // Using FromISR variant here as it's not that safe to call normal FreeRTOS APIs
    // under a critical section/disabled interrupts.
    int r = xQueueSendFromISR((QueueHandle_t)queue_, &ptr, nullptr);
    if (r != pdPASS) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    return 0;
}

} // particle
