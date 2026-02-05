/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include <memory>
#include <mutex>
#include <atomic>
#include <functional>
#include "delay_hal.h"
#include "unique_lock.h"
#include "service_debug.h"
// FIXME: app_util.h unconditionally defines STATIC_ASSERT on Gen 3 platforms
#ifdef STATIC_ASSERT
#undef STATIC_ASSERT
#endif
#include "static_mutex.h"

namespace particle {

extern StaticMutex& OnceMutex();

enum class OnceState : uint8_t {
    NOT_INITIALIZED = 0,
    RUNNING = 1,
    INITIALIZED = 2
};

struct OnceFlag {
    constexpr OnceFlag() noexcept = default;

    OnceFlag(const OnceFlag&) = delete;
    OnceFlag& operator=(const OnceFlag&) = delete;

private:
    std::atomic<OnceState> state = OnceState::NOT_INITIALIZED;
    static_assert(decltype(state)::is_always_lock_free);

    template<typename CallableT, typename... Args>
    friend void CallOnce(OnceFlag& once, CallableT&& callable, Args&&... args);
};

template<typename CallableT, typename... Args>
void CallOnce(OnceFlag& once, CallableT&& callable, Args&&... args) {

    if (once.state.load(std::memory_order_acquire) == OnceState::INITIALIZED) {
        // Short path, already initialized
        return;
    }

    UniqueLock lk(OnceMutex());

    auto state = once.state.load(std::memory_order_acquire);
    if (state == OnceState::INITIALIZED) {
        return;
    }

    if (state == OnceState::NOT_INITIALIZED) {
        once.state.store(OnceState::RUNNING, std::memory_order_relaxed);
        lk.unlock();
        auto f = [&] {
            std::invoke(std::forward<CallableT>(callable), std::forward<Args>(args)...);
        };
        f();
        once.state.store(OnceState::INITIALIZED, std::memory_order_relaxed);
    } else {
        lk.unlock();
        // Not ideal, but this mimics our prior gthread implementation
        while (once.state.load(std::memory_order_acquire) == OnceState::RUNNING) {
            HAL_Delay_Milliseconds(0);
        }
    }
}

} // namespace particle
