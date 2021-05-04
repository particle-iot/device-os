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

#include "test_suite.h"

#include "platform_headers.h"
#include "check.h"

#include "spark_wiring_system.h"

#include "unit-test/unit-test.h"

void system_initialize_user_backup_ram(); // Defined in user.cpp

namespace particle {

namespace {

const uint32_t MAGIC_NUMBER = 0x0ff1c1a1;

struct CurrentConfig {
    size_t size;
    uint32_t magicNumber;
    System_Mode_TypeDef systemMode;
    bool systemThreadEnabled;
};

retained CurrentConfig g_config;

} // namespace

TestSuite::TestSuite() :
        inited_(false) {
}

TestSuite::~TestSuite() {
    destroy();
}

int TestSuite::init() {
    // Enable backup memory
    System.enableFeature(FEATURE_RETAINED_MEMORY);
    // Set system mode
    if (g_config.size != sizeof(CurrentConfig) || g_config.magicNumber != MAGIC_NUMBER) {
        g_config.size = sizeof(CurrentConfig);
        g_config.magicNumber = MAGIC_NUMBER;
        g_config.systemMode = TestSuiteConfig::DEFAULT_SYSTEM_MODE;
        g_config.systemThreadEnabled = TestSuiteConfig::DEFAULT_SYSTEM_THREAD_ENABLED;
    }
    set_system_mode(g_config.systemMode);
    system_thread_set_state(g_config.systemThreadEnabled ? spark::feature::ENABLED : spark::feature::DISABLED, nullptr);
    // Configure test runner
    const auto runner = TestRunner::instance();
    runner->cloudEnabled(false);
    runner->serialEnabled(false);
    runner->ledEnabled(false);
    runner->logEnabled(true);
    Test::max_verbosity = TEST_VERBOSITY_ASSERTIONS_FAILED;
    // Initialize request handler
    CHECK(reqHandler_.init());
    inited_ = true;
    return 0;
}

void TestSuite::destroy() {
    reqHandler_.destroy();
    memset(&g_config, 0, sizeof(g_config));
    inited_ = false;
}

int TestSuite::config(const TestSuiteConfig& config) {
    CHECK_TRUE(inited_, SYSTEM_ERROR_INVALID_STATE);
    int result = 0;
    if (g_config.systemMode != config.systemMode() || g_config.systemThreadEnabled != config.systemThreadEnabled()) {
        g_config.systemMode = config.systemMode();
        g_config.systemThreadEnabled = config.systemThreadEnabled();
        result = Result::RESET_PENDING;
    }
    if (config.clearBackupMemory()) {
        const auto config = g_config;
        system_initialize_user_backup_ram();
        g_config = config;
        result = Result::RESET_PENDING;
    }
    return result;
}

TestSuite* TestSuite::instance() {
    static TestSuite suite;
    return &suite;
}

} // namespace particle

void ctrl_request_custom_handler(ctrl_request* req) {
    const auto handler = particle::TestSuite::instance()->requestHandler();
    handler->process(req);
}
