/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include "system_mode.h"

namespace particle {

namespace test {

// Custom result codes
enum Result {
    RESET_PENDING = 1
};

class SuiteConfig {
public:
    static const System_Mode_TypeDef DEFAULT_SYSTEM_MODE = MANUAL;
    static const bool DEFAULT_SYSTEM_THREAD_ENABLED = false;

    SuiteConfig();

    SuiteConfig& systemMode(System_Mode_TypeDef mode);
    System_Mode_TypeDef systemMode() const;

    SuiteConfig& systemThreadEnabled(bool enabled);
    bool systemThreadEnabled() const;

private:
    System_Mode_TypeDef systemMode_;
    bool systemThreadEnabled_;
};

class Suite {
public:
    ~Suite();

    int init();
    void destroy();

    int config(const SuiteConfig& config);

    static Suite* instance();

private:
    Suite();

    bool inited_;
};

inline SuiteConfig::SuiteConfig() :
        systemMode_(DEFAULT_SYSTEM_MODE),
        systemThreadEnabled_(DEFAULT_SYSTEM_THREAD_ENABLED) {
}

inline SuiteConfig& SuiteConfig::systemMode(System_Mode_TypeDef mode) {
    systemMode_ = mode;
    return *this;
}

inline System_Mode_TypeDef SuiteConfig::systemMode() const {
    return systemMode_;
}

inline SuiteConfig& SuiteConfig::systemThreadEnabled(bool enabled) {
    systemThreadEnabled_ = enabled;
    return *this;
}

inline bool SuiteConfig::systemThreadEnabled() const {
    return systemThreadEnabled_;
}

} // namespace test

} // namespace particle
