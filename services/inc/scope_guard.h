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

#include <utility>

// TODO: Add a separate class for unnamed scope guards
#define SCOPE_GUARD(_func) \
        NAMED_SCOPE_GUARD(_scope_guard_##__COUNTER__, _func)

#define NAMED_SCOPE_GUARD(_name, _func) \
        auto _name = ::particle::makeNamedScopeGuard([&] _func)

namespace particle {

template<typename FuncT>
class NamedScopeGuard {
public:
    NamedScopeGuard(FuncT&& func) :
            func_(std::move(func)),
            dismissed_(false) {
    }

    NamedScopeGuard(NamedScopeGuard&& guard) :
            func_(std::move(guard.func_)),
            dismissed_(guard.dismissed_) {
        guard.dismissed_ = true;
    }

    ~NamedScopeGuard() {
        if (!dismissed_) {
            func_();
        }
    }

    void dismiss() {
        dismissed_ = true;
    }

    NamedScopeGuard(const NamedScopeGuard&) = delete;
    NamedScopeGuard& operator=(const NamedScopeGuard&) = delete;

private:
    const FuncT func_;
    bool dismissed_;
};

template<typename FuncT>
inline NamedScopeGuard<FuncT> makeNamedScopeGuard(FuncT&& func) {
    return NamedScopeGuard<FuncT>(std::move(func));
}

} // particle
