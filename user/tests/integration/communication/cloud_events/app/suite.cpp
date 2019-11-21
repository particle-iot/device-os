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

#include "suite.h"

#include "request_handler.h"

#include "platform_headers.h"
#include "check.h"

#include "unit-test/unit-test.h"

namespace particle {

namespace test {

namespace {

using namespace spark;

const uint32_t MAGIC_NUMBER = 0x0ff1c1a1;

struct CurrentSuiteConfig {
    size_t size;
    uint32_t magicNumber;
    System_Mode_TypeDef systemMode;
    bool systemThreadEnabled;
};

retained CurrentSuiteConfig g_config;

} // namespace

Suite::Suite() :
        inited_(false) {
}

Suite::~Suite() {
    destroy();
}

int Suite::init() {
    // Set system mode
    if (g_config.size != sizeof(CurrentSuiteConfig) || g_config.magicNumber != MAGIC_NUMBER) {
        g_config.size = sizeof(CurrentSuiteConfig);
        g_config.magicNumber = MAGIC_NUMBER;
        g_config.systemMode = SuiteConfig::DEFAULT_SYSTEM_MODE;
        g_config.systemThreadEnabled = SuiteConfig::DEFAULT_SYSTEM_THREAD_ENABLED;
    }
    set_system_mode(g_config.systemMode);
    system_thread_set_state(g_config.systemThreadEnabled ? feature::ENABLED : feature::DISABLED, nullptr);
    // Initialize request handler
    CHECK(RequestHandler::instance()->init());
    // Configure test runner
    const auto runner = TestRunner::instance();
    runner->ledEnabled(false);
    runner->serialEnabled(false);
    runner->cloudEnabled(false);
    runner->logEnabled(true);
    inited_ = true;
    return 0;
}

void Suite::destroy() {
    RequestHandler::instance()->destroy();
    memset(&g_config, 0, sizeof(g_config));
    inited_ = false;
}

int Suite::config(const SuiteConfig& config) {
    CHECK_TRUE(inited_, SYSTEM_ERROR_INVALID_STATE);
    if (g_config.systemMode != config.systemMode() || g_config.systemThreadEnabled != config.systemThreadEnabled()) {
        g_config.systemMode = config.systemMode();
        g_config.systemThreadEnabled = config.systemThreadEnabled();
        return Result::RESET_PENDING;
    }
    return 0;
}

Suite* Suite::instance() {
    static Suite suite;
    return &suite;
}

} // namespace test

} // namespace particle
