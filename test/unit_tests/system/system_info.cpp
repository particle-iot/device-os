/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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


#include "system_update.h"
#include "system_cloud_internal.h"
#include <hippomocks.h>
#include <iostream>
#include <string>
#include <vector>
#include "mock/system_info_mock.h"
#include "spark_wiring_json.h"
#include <limits>
#include "hex_to_bytes.h"
#include "util/string_appender.h"
#include <map>

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

using namespace particle;

namespace {

struct JsonObjectAppenderWrapper: public BufferAppender {
    char buf[1024 * 1024] = {};

    JsonObjectAppenderWrapper()
            : BufferAppender(buf, sizeof(buf) - 1) {
        appendChar('{');
    }

    void finish() {
        appendChar('}');
    }
};

bool parseModulesJson(const spark::JSONValue& modules, std::vector<hal_module_t>& out) {
    using namespace spark;
    using namespace particle::system;
    JSONArrayIterator iter(modules);
    while (iter.next()) {
        CHECK(iter.value().isObject());
        JSONObjectIterator modIter(iter.value());
        hal_module_t parsed = {};
        bool hasMaxSize = false;
        bool hasStore = false;
        bool hasValidityChecked = false;
        bool hasValidityResult = false;
        bool hasFunction = false;
        bool hasIndex = false;
        bool hasVersion = false;
        bool hasDependencies = false;
        bool hasUuid = false;
        while (modIter.next()) {
            if (modIter.name() == "s") {
                CHECK(modIter.value().isNumber());
                parsed.bounds.maximum_size = modIter.value().toInt();
                hasMaxSize = true;
            } else if (modIter.name() == "l") {
                CHECK(modIter.value().isString());
                auto v = module_store_from_string(modIter.value().toString().data());
                if (v < 0) {
                    v = std::numeric_limits<decltype(parsed.bounds.store)>::max();
                }
                parsed.bounds.store = (module_store_t)v;
                hasStore = true;
            } else if (modIter.name() == "vc") {
                CHECK(modIter.value().isNumber());
                parsed.validity_checked = modIter.value().toInt();
                hasValidityChecked = true;
            } else if (modIter.name() == "vv") {
                CHECK(modIter.value().isNumber());
                parsed.validity_result = modIter.value().toInt();
                hasValidityResult = true;
            } else if (modIter.name() == "f") {
                CHECK(modIter.value().isString());
                auto v = module_function_from_string(modIter.value().toString().data());
                if (v < 0) {
                    v = MODULE_FUNCTION_MAX;
                }
                parsed.info.module_function = (module_function_t)v;
                hasFunction = true;
            } else if (modIter.name() == "n") {
                // NOTE: yes, module index is supposed to be a string not a Number
                CHECK(modIter.value().isString());
                parsed.info.module_index = std::stoul(modIter.value().toString().data());
                hasIndex = true;
            } else if (modIter.name() == "v") {
                CHECK(modIter.value().isNumber());
                parsed.info.module_version = modIter.value().toInt();
                hasVersion = true;
            } else if (modIter.name() == "d") {
                CHECK(modIter.value().isArray());
                JSONArrayIterator deps(modIter.value());
                CHECK(deps.count() <= 2);
                hasDependencies = true;
                int i = 0;
                while (deps.next()) {
                    // First dependency
                    bool depHasFunction = false;
                    bool depHasIndex = false;
                    bool depHasVersion = false;
                    CHECK(deps.value().isObject());
                    JSONObjectIterator depIter(deps.value());
                    while (depIter.next()) {
                        if (depIter.name() == "f") {
                            CHECK(depIter.value().isString());
                            auto v = module_function_from_string(depIter.value().toString().data());
                            if (v < 0) {
                                v = MODULE_FUNCTION_MAX;
                            }
                            if (i == 0) {
                                parsed.info.dependency.module_function = (module_function_t)v;
                            } else {
                                parsed.info.dependency2.module_function = (module_function_t)v;
                            }
                            depHasFunction = true;
                        } else if (depIter.name() == "n") {
                            // FIXME: is this really supposed to be a string?
                            // CHECK(depIter.value().isNumber());
                            // unsigned v = depIter.value().toInt();
                            CHECK(depIter.value().isString());
                            unsigned v = std::stoul(depIter.value().toString().data());

                            if (i == 0) {
                                parsed.info.dependency.module_index = v;
                            } else {
                                parsed.info.dependency2.module_index = v;
                            }
                            depHasIndex = true;
                        } else if (depIter.name() == "v") {
                            CHECK(depIter.value().isNumber());
                            if (i == 0) {
                                parsed.info.dependency.module_version = depIter.value().toInt();
                            } else {
                                parsed.info.dependency2.module_version = depIter.value().toInt();
                            }
                            depHasVersion = true;
                        }
                    }
                    CHECK(depHasFunction);
                    CHECK(depHasIndex);
                    CHECK(depHasVersion);
                    i++;
                }
            } else if (modIter.name() == "u") {
                CHECK(modIter.value().isString());
                auto s = modIter.value().toString();
                CHECK(s.size() == sizeof(parsed.suffix.sha) * 2);
                hexToBytes(s.data(), (char*)parsed.suffix.sha, s.size());
                hasUuid = true;
            }
        }
        CHECK(hasMaxSize);
        CHECK(hasStore);
        CHECK(hasValidityChecked);
        CHECK(hasValidityResult);
        CHECK(hasFunction);
        CHECK(hasIndex);
        CHECK(hasVersion);
        CHECK(hasDependencies);
        if (parsed.info.module_function == MODULE_FUNCTION_USER_PART) {
            CHECK(hasUuid);
        }
        out.push_back(parsed);
    }
    return true;
}

struct ParsedSystemInfo {
    int platformId = -1;
    std::vector<hal_module_t> modules;
    std::map<std::string, std::string> kv;
};

ParsedSystemInfo parseSystemInfoJson(const std::string& json) {
    using namespace spark;
    auto obj = JSONValue::parseCopy(json.c_str(), json.length());
    CHECK(obj.isValid());
    CHECK(obj.isObject());
    JSONObjectIterator iter(obj);
    ParsedSystemInfo info;
    bool hasModules = false;
    while (iter.next()) {
        if (iter.name() == "p") {
            CHECK(iter.value().isNumber());
            info.platformId = iter.value().toInt();
        } else if (iter.name() == "m") {
            hasModules = true;
            CHECK(iter.value().isArray());
            CHECK(parseModulesJson(iter.value(), info.modules));
        } else {
            CHECK(iter.value().isString());
            info.kv[iter.name().data()] = iter.value().toString().data();
        }
    }
    CHECK(info.platformId >= 0);
    CHECK(hasModules);
    return info;
}

bool modulesMatch(const hal_module_t& parsed, const hal_module_t& expected) {
    using namespace particle::system;
    if (parsed.bounds.maximum_size != expected.bounds.maximum_size) {
        return false;
    }
    if (parsed.bounds.store != expected.bounds.store) {
        return false;
    }
    if (parsed.validity_checked != expected.validity_checked) {
        return false;
    }
    if (parsed.validity_result != expected.validity_result) {
        return false;
    }
    if (parsed.info.module_function != expected.info.module_function) {
        return false;
    }
    if (parsed.info.module_index != expected.info.module_index) {
        return false;
    }
    if (parsed.info.module_version != expected.info.module_version) {
        return false;
    }
    module_dependency_t dep1 = {};
    module_dependency_t dep2 = {};
    // Oh boy, there should be a better way to do this
    if (is_module_function_valid((module_function_t)expected.info.dependency.module_function) &&
            is_module_function_valid((module_function_t)expected.info.dependency2.module_function)) {
        // Both dependencies present, both should match
        dep1 = expected.info.dependency;
        dep2 = expected.info.dependency2;
    }
    else if (is_module_function_valid((module_function_t)expected.info.dependency.module_function)) {
        // First dependency present only, should match
        dep1 = expected.info.dependency;
    } else if (is_module_function_valid((module_function_t)expected.info.dependency2.module_function)) {
        // Second dependency present only
        dep1 = expected.info.dependency2;
    }
    if (parsed.info.dependency.module_function != dep1.module_function) {
        return false;
    }
    if (parsed.info.dependency.module_index != dep1.module_index) {
        return false;
    }
    if (parsed.info.dependency.module_version != dep1.module_version) {
        return false;
    }
    if (parsed.info.dependency2.module_function != dep2.module_function) {
        return false;
    }
    if (parsed.info.dependency2.module_index != dep2.module_index) {
        return false;
    }
    if (parsed.info.dependency2.module_version != dep2.module_version) {
        return false;
    }
    if (parsed.info.module_function == MODULE_FUNCTION_USER_PART) {
        if (memcmp(parsed.suffix.sha, expected.suffix.sha, sizeof(parsed.suffix.sha))) {
            return false;
        }
    }
    return true;
}

bool validateSystemInfoJson(const std::string& json, const particle::test::SystemInfo& systemInfo) {
    INFO(json);
    auto info = parseSystemInfoJson(json);
    auto expectedModules = systemInfo.getOnlyPresentModules();
    auto expectedKv = systemInfo.getKeyValues();
    CHECK(info.platformId == systemInfo.getPlatformId());
    CHECK(info.modules.size() == expectedModules.size());
    CHECK(info.kv.size() == expectedKv.size());
    for (const auto& m: info.modules) {
        bool found = false;
        for (auto it = expectedModules.begin(); it != expectedModules.end(); it++) {
            const auto& expected = *it;
            if (modulesMatch(m, expected)) {
                found = true;
                expectedModules.erase(it);
                break;
            }
        }
        CHECK(found);
    }
    for (const auto& kv: expectedKv) {
        auto it = info.kv.find(kv.key);
        CHECK(it != info.kv.end());
        CHECK(it->second == kv.value);
    }
    return true;
}

const unsigned RANDOM_TEST_CASES = 100;

} // anonymous

TEST_CASE("system_module_info") {
    MockRepository mocks;
    particle::test::SystemInfo systemInfo(&mocks);
    JsonObjectAppenderWrapper appender;

    using namespace particle::system;

    SECTION("canonical gen3 cellular device info") {
        const char describe[] = "{\"p\":23,\"imei\":\"354724645624658\",\"iccid\":\"89014103273166754725\",\"cellfw\":\"L0.0.00.00.05.08,A.02.04\","
            "\"m\":[{\"s\":49152,\"l\":\"m\",\"vc\":30,\"vv\":30,\"f\":\"b\",\"n\":\"0\",\"v\":1005,\"d\":[]},{\"s\":671744,\"l\":\"m\",\"vc\":30,\"vv\":30,"
            "\"f\":\"s\",\"n\":\"1\",\"v\":2101,\"d\":[{\"f\":\"b\",\"n\":\"0\",\"v\":1005,\"_\":\"\"},{\"f\":\"a\",\"n\":\"0\",\"v\":202,\"_\":\"\"}]},"
            "{\"s\":131072,\"l\":\"m\",\"vc\":30,\"vv\":30,\"u\":\"1A9C70EF07106EB9B2B8076708A3ACB8390F14ED4C5066DA44FC8260F3CFBEF1\",\"f\":\"u\",\"n\":\"1\","
            "\"v\":6,\"d\":[{\"f\":\"s\",\"n\":\"1\",\"v\":2011,\"_\":\"\"}]},{\"s\":192512,\"l\":\"m\",\"vc\":30,\"vv\":30,\"f\":\"a\",\"n\":\"0\",\"v\":202,\"d\":[]}]}";

        auto parsed = parseSystemInfoJson(describe);
        CHECK(parsed.platformId == 23);
        CHECK(parsed.modules.size() == 4);
        CHECK(parsed.kv.size() == 3);
        systemInfo.setPlatformId(parsed.platformId);
        for (const auto& m: parsed.modules) {
            systemInfo.addModule(m);
        }
        for (const auto& kv: parsed.kv) {
            systemInfo.addKeyValue(kv.first, kv.second);
        }
        CHECK(systemInfo.getOnlyPresentModules().size() == parsed.modules.size());
        CHECK(validateSystemInfoJson(describe, systemInfo));

        CHECK(system_module_info(appender.callback, &appender, nullptr));
        appender.finish();

        CHECK(validateSystemInfoJson(appender.buffer(), systemInfo));
    }

    SECTION("canonical argon info") {
        const char describe[] = "{\"p\":12,\"m\":[{\"s\":49152,\"l\":\"m\",\"vc\":30,\"vv\":30,\"f\":\"b\",\"n\":\"0\",\"v\":501,"
            "\"d\":[]},{\"s\":671744,\"l\":\"m\",\"vc\":30,\"vv\":30,\"f\":\"s\",\"n\":\"1\",\"v\":1512,\"d\":[{\"f\":\"b\",\"n\":\"0\","
            "\"v\":501,\"_\":\"\"},{\"f\":\"a\",\"n\":\"0\",\"v\":202,\"_\":\"\"}]},{\"s\":131072,\"l\":\"m\",\"vc\":30,\"vv\":30,\"u\":"
            "\"CD5AFF31423B4AF1DAE1684481E29C91FB2438BD740B253737F9B145C81D0078\",\"f\":\"u\",\"n\":\"1\",\"v\":6,\"d\":[{\"f\":\"s\",\"n\":"
            "\"1\",\"v\":1512,\"_\":\"\"}]},{\"s\":1536000,\"l\":\"m\",\"vc\":30,\"vv\":30,\"f\":\"c\",\"n\":\"0\",\"v\":5,\"d\":[]},{\"s\":192512,"
            "\"l\":\"m\",\"vc\":30,\"vv\":30,\"f\":\"a\",\"n\":\"0\",\"v\":202,\"d\":[]}]}";
        auto parsed = parseSystemInfoJson(describe);
        CHECK(parsed.platformId == 12);
        CHECK(parsed.modules.size() == 5);
        CHECK(parsed.kv.size() == 0);
        systemInfo.setPlatformId(parsed.platformId);
        for (const auto& m: parsed.modules) {
            systemInfo.addModule(m);
        }
        for (const auto& kv: parsed.kv) {
            systemInfo.addKeyValue(kv.first, kv.second);
        }
        CHECK(systemInfo.getOnlyPresentModules().size() == parsed.modules.size());
        CHECK(validateSystemInfoJson(describe, systemInfo));

        CHECK(system_module_info(appender.callback, &appender, nullptr));
        appender.finish();

        CHECK(validateSystemInfoJson(appender.buffer(), systemInfo));
    }

    SECTION("canonical tracker info") {
        const char describe[] = "{\"p\":26,\"imei\":\"860112043167735\",\"iccid\":\"89014103272880448051\",\"m\":[{\"s\":49152,\"l\":\"m\""
            ",\"vc\":30,\"vv\":30,\"f\":\"b\",\"n\":\"0\",\"v\":1005,\"d\":[]},{\"s\":671744,\"l\":\"m\",\"vc\":30,\"vv\":30,\"f\":\"s\","
            "\"n\":\"1\",\"v\":3000,\"d\":[{\"f\":\"b\",\"n\":\"0\",\"v\":1005,\"_\":\"\"},{\"f\":\"a\",\"n\":\"0\",\"v\":202,\"_\":\"\"}]},"
            "{\"s\":131072,\"l\":\"m\",\"vc\":30,\"vv\":26,\"u\":\"F17CB35A16D233DA7A9601C8572B67EE85F9968067438A1B426C66A793A3A68E\",\"f\":"
            "\"u\",\"n\":\"1\",\"v\":6,\"d\":[{\"f\":\"s\",\"n\":\"1\",\"v\":3005,\"_\":\"\"}]},{\"s\":1536000,\"l\":\"m\",\"vc\":30,\"vv\":30,"
            "\"f\":\"c\",\"n\":\"0\",\"v\":7,\"d\":[]},{\"s\":192512,\"l\":\"m\",\"vc\":30,\"vv\":30,\"f\":\"a\",\"n\":\"0\",\"v\":202,\"d\":[]}]}";
        auto parsed = parseSystemInfoJson(describe);
        CHECK(parsed.platformId == 26);
        CHECK(parsed.modules.size() == 5);
        CHECK(parsed.kv.size() == 2);
        systemInfo.setPlatformId(parsed.platformId);
        for (const auto& m: parsed.modules) {
            systemInfo.addModule(m);
        }
        for (const auto& kv: parsed.kv) {
            systemInfo.addKeyValue(kv.first, kv.second);
        }
        CHECK(systemInfo.getOnlyPresentModules().size() == parsed.modules.size());
        CHECK(validateSystemInfoJson(describe, systemInfo));

        CHECK(system_module_info(appender.callback, &appender, nullptr));
        appender.finish();

        CHECK(validateSystemInfoJson(appender.buffer(), systemInfo));
    }

    SECTION("empty module list") {
        systemInfo.setPlatformId(123);
        CHECK(system_module_info(appender.callback, &appender, nullptr));
        appender.finish();

        CHECK(validateSystemInfoJson(appender.buffer(), systemInfo));
    }
    for (unsigned type = MODULE_FUNCTION_NONE; type <= MODULE_FUNCTION_MAX; type++) {
        if (is_module_function_valid((module_function_t)type)) {
            for (unsigned i = 0; i < RANDOM_TEST_CASES; i++) {
                SECTION(std::to_string(i) + " single random module f:" + module_function_string((module_function_t)type)) {
                    systemInfo.setPlatformId(123);
                    systemInfo.addModule((module_function_t)type);
                    CHECK(system_module_info(appender.callback, &appender, nullptr));
                    appender.finish();

                    CHECK(validateSystemInfoJson(appender.buffer(), systemInfo));
                }
            }
        }
    }

    for (unsigned i = 0; i < RANDOM_TEST_CASES; i++) {
        SECTION(std::to_string(i) + " five random modules") {
            using namespace particle::test;
            systemInfo.setPlatformId(123);
            systemInfo.addModule(genValidOrNoneModuleFunction());
            systemInfo.addModule(genValidOrNoneModuleFunction());
            systemInfo.addModule(genValidOrNoneModuleFunction());
            systemInfo.addModule(genValidOrNoneModuleFunction());
            systemInfo.addModule(genValidOrNoneModuleFunction());
            CHECK(system_module_info(appender.callback, &appender, nullptr));
            appender.finish();

            CHECK(validateSystemInfoJson(appender.buffer(), systemInfo));
        }
    }

    for (unsigned i = 0; i < RANDOM_TEST_CASES; i++) {
        SECTION(std::to_string(i) + " five random modules with five extra key values") {
            using namespace particle::test;
            systemInfo.setPlatformId(123);
            systemInfo.addModule(genValidOrNoneModuleFunction());
            systemInfo.addModule(genValidOrNoneModuleFunction());
            systemInfo.addModule(genValidOrNoneModuleFunction());
            systemInfo.addModule(genValidOrNoneModuleFunction());
            systemInfo.addModule(genValidOrNoneModuleFunction());
            systemInfo.addKeyValue();
            systemInfo.addKeyValue();
            systemInfo.addKeyValue();
            systemInfo.addKeyValue();
            systemInfo.addKeyValue();
            CHECK(system_module_info(appender.callback, &appender, nullptr));
            appender.finish();

            CHECK(validateSystemInfoJson(appender.buffer(), systemInfo));
        }
    }

    SECTION("valid module in factory location should be present") {
        using namespace particle::test;
        systemInfo.setPlatformId(123);
        systemInfo.addModule(MODULE_FUNCTION_USER_PART);
        auto factory = genRandomModule(MODULE_FUNCTION_USER_PART, MODULE_STORE_FACTORY);
        factory.validity_checked = MODULE_VALIDATION_END;
        factory.validity_result = MODULE_VALIDATION_END;
        systemInfo.addModule(factory);
        CHECK(systemInfo.getOnlyPresentModules().size() == 2);

        CHECK(system_module_info(appender.callback, &appender, nullptr));
        appender.finish();

        CHECK(validateSystemInfoJson(appender.buffer(), systemInfo));
    }

    SECTION("invalid module in factory location should not be present") {
        using namespace particle::test;
        systemInfo.setPlatformId(123);
        systemInfo.addModule(MODULE_FUNCTION_USER_PART);
        auto factory = genRandomModule(MODULE_FUNCTION_USER_PART, MODULE_STORE_FACTORY);
        factory.validity_checked = MODULE_VALIDATION_END;
        factory.validity_result = 0;
        systemInfo.addModuleThatShouldNotBePresentInJson(factory);
        CHECK(systemInfo.getOnlyPresentModules().size() == 1);

        CHECK(system_module_info(appender.callback, &appender, nullptr));
        appender.finish();

        CHECK(validateSystemInfoJson(appender.buffer(), systemInfo));
    }

}

TEST_CASE("system_app_info") {
    MockRepository mocks;
    test::StringAppender appender;

    SECTION("without functions and variables") {
        mocks.OnCallFunc(cloudFunctionCount).Do([]() {
            return 0;
        });
        mocks.OnCallFunc(cloudVariableCount).Do([]() {
            return 0;
        });
        CHECK(system_app_info(appender.callback, &appender, nullptr));
        CHECK(appender.data() == "\"f\":[],\"v\":{}");
    }

    SECTION("with functions and variables") {
        mocks.OnCallFunc(cloudFunctionCount).Do([]() {
            return 1;
        });
        mocks.OnCallFunc(getCloudFunctionInfo).Do([](size_t index, const char** name) {
            switch (index) {
                case 0: *name = "fn1"; return 0;
                default: return -1;
            }
        });
        mocks.OnCallFunc(cloudVariableCount).Do([]() {
            return 2;
        });
        mocks.OnCallFunc(getCloudVariableInfo).Do([](size_t index, const char** name, int* type) {
            switch (index) {
                case 0: *name = "var1"; *type = 1; return 0;
                case 1: *name = "var2"; *type = 2; return 0;
                default: return -1;
            }
        });
        CHECK(system_app_info(appender.callback, &appender, nullptr));
        CHECK(appender.data() == "\"f\":[\"fn1\"],\"v\":{\"var1\":1,\"var2\":2}");
    }
}
