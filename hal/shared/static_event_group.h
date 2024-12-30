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
#include "hal_platform.h"
#include <FreeRTOS.h>
#include <event_groups.h>
#include "system_tick_hal.h"
#include "enumflags.h"

class StaticEventGroup {
public:
    enum class Flag {
        NONE = 0x00,
        CLEAR_ON_EXIT = 0x01,
        WAIT_ALL = 0x02
    };

    typedef particle::EnumFlags<Flag> Flags;

    StaticEventGroup() {
        handle_ = xEventGroupCreateStatic(&eventGroupBuffer_);
        SPARK_ASSERT(handle_);
    }

    ~StaticEventGroup() {
        vEventGroupDelete(handle_);
    }

    uint32_t wait(uint32_t bits, system_tick_t wait = portMAX_DELAY, Flags f = Flag::NONE) {
        SPARK_ASSERT(!hal_interrupt_is_isr());
        return xEventGroupWaitBits(handle_, bits, f & Flag::CLEAR_ON_EXIT, f & Flag::WAIT_ALL, wait);
    }

    uint32_t wait(uint32_t bits, Flags f = Flag::NONE) {
        return wait(bits, portMAX_DELAY, f);
    }

    uint32_t clear(uint32_t bits) {
        SPARK_ASSERT(!hal_interrupt_is_isr());
        return xEventGroupClearBits(handle_, bits);
    }

    uint32_t set(uint32_t bits) {
        SPARK_ASSERT(!hal_interrupt_is_isr());
        return xEventGroupSetBits(handle_, bits);
    }

    uint32_t sync(uint32_t setBits, uint32_t waitBits, system_tick_t wait = portMAX_DELAY) {
        SPARK_ASSERT(!hal_interrupt_is_isr());
        return xEventGroupSync(handle_, setBits, waitBits, wait);
    }

private:
    EventGroupHandle_t handle_;
    StaticEventGroup_t eventGroupBuffer_;
};

namespace particle {

ENABLE_ENUM_CLASS_BITWISE(StaticEventGroup::Flag);

} // particle
