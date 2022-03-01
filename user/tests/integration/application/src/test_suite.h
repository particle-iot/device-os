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

#include "request_handler.h"

#include "system_mode.h"

namespace particle {

// Custom result codes
enum Result {
    STATUS_PASSED = 1,
    STATUS_FAILED = 2,
    STATUS_SKIPPED = 3,
    STATUS_RUNNING = 4,
    STATUS_WAITING = 5,
    RESET_PENDING = 6
};

class TestSuiteConfig {
public:
    static const System_Mode_TypeDef DEFAULT_SYSTEM_MODE = SEMI_AUTOMATIC;
    static const bool DEFAULT_SYSTEM_THREAD_ENABLED = false;

    TestSuiteConfig();

    TestSuiteConfig& systemMode(System_Mode_TypeDef mode);
    System_Mode_TypeDef systemMode() const;

    TestSuiteConfig& systemThreadEnabled(bool enabled);
    bool systemThreadEnabled() const;

    TestSuiteConfig& clearBackupMemory(bool enabled);
    bool clearBackupMemory() const;

private:
    System_Mode_TypeDef systemMode_;
    bool systemThreadEnabled_;
    bool clearBackupMemory_;
};

class TestSuite {
public:
    ~TestSuite();

    int init();
    void destroy();

    RequestHandler* requestHandler();

    int config(const TestSuiteConfig& config);

    static TestSuite* instance();

private:
    TestSuite();

    RequestHandler reqHandler_;
    bool inited_;
};

inline TestSuiteConfig::TestSuiteConfig() :
        systemMode_(DEFAULT_SYSTEM_MODE),
        systemThreadEnabled_(DEFAULT_SYSTEM_THREAD_ENABLED),
        clearBackupMemory_(false) {
}

inline TestSuiteConfig& TestSuiteConfig::systemMode(System_Mode_TypeDef mode) {
    systemMode_ = mode;
    return *this;
}

inline System_Mode_TypeDef TestSuiteConfig::systemMode() const {
    return systemMode_;
}

inline TestSuiteConfig& TestSuiteConfig::systemThreadEnabled(bool enabled) {
    systemThreadEnabled_ = enabled;
    return *this;
}

inline bool TestSuiteConfig::systemThreadEnabled() const {
    return systemThreadEnabled_;
}

inline TestSuiteConfig& TestSuiteConfig::clearBackupMemory(bool enabled) {
    clearBackupMemory_ = enabled;
    return *this;
}

inline bool TestSuiteConfig::clearBackupMemory() const {
    return clearBackupMemory_;
}

inline RequestHandler* TestSuite::requestHandler() {
    return &reqHandler_;
}

} // namespace particle
