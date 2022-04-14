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

#pragma once

#include <hippomocks.h>

#include <vector>
#include <string>
#include "system_info.h"
#include "util/random.h"

namespace particle {

namespace test {

inline module_function_t genValidOrNoneModuleFunction() {
    while (true) {
        auto func = (module_function_t)(randNumber<uint8_t>() % MODULE_FUNCTION_MAX);
        if (particle::system::is_module_function_valid(func) || func == MODULE_FUNCTION_NONE) {
            return func;
        }
    }
}

inline hal_module_t genRandomModule(module_function_t function, module_store_t store = MODULE_STORE_MAIN) {
    hal_module_t module = {};

    // bounds
    module.bounds.maximum_size = randNumber(module.bounds.maximum_size);
    module.bounds.start_address = randNumber(module.bounds.start_address);
    module.bounds.end_address = randNumber(module.bounds.end_address);
    module.bounds.module_function = function;
    module.bounds.module_index = randNumber(module.bounds.module_index);
    module.bounds.store = store;
    module.bounds.mcu_identifier = 1;
    // Don't matter
    module.bounds.location = MODULE_BOUNDS_LOC_INTERNAL_FLASH;

    // info
    module.info.module_start_address = (void*)((uintptr_t)module.bounds.start_address);
    module.info.module_end_address = (void*)((uintptr_t)module.bounds.end_address);
    module.info.reserved = randNumber(module.info.reserved);
    module.info.flags = randNumber(module.info.flags);
    module.info.module_version = randNumber(module.info.module_version);
    module.info.module_function = function;
    module.info.module_index = module.bounds.module_index;
    // dependency
    module.info.dependency.module_function = genValidOrNoneModuleFunction();
    module.info.dependency.module_index = randNumber(module.info.dependency.module_index);
    module.info.dependency.module_version = randNumber(module.info.dependency.module_version);
    // dependency2
    module.info.dependency2.module_function = genValidOrNoneModuleFunction();
    module.info.dependency2.module_index = randNumber(module.info.dependency2.module_index);
    module.info.dependency2.module_version = randNumber(module.info.dependency2.module_version);
    module.crc.crc32 = randNumber(module.crc.crc32);
    // suffix
    module.suffix.reserved = 0;
    memcpy(module.suffix.sha, randString(sizeof(module.suffix.sha)).c_str(), sizeof(module.suffix.sha));
    module.suffix.size = sizeof(module.suffix);

    // validity
    module.validity_checked = randNumber(module.validity_checked) & MODULE_VALIDATION_END;
    module.validity_result = randNumber(module.validity_result) & MODULE_VALIDATION_END;

    return module;
}

class SystemInfo {
public:
    explicit SystemInfo(MockRepository* mocks);
    ~SystemInfo() noexcept(false);

    struct KeyValue: public key_value {
        KeyValue(const std::string& key, const std::string& value) {
            memset(this->value, 0, sizeof(this->value));
            this->key = strdup(key.c_str());
            strncpy(this->value, value.c_str(), sizeof(this->value) - 1);
        }
        KeyValue(const KeyValue& other)
                : KeyValue(other.key, other.value) {
        }
        KeyValue& operator=(const KeyValue& other) {
            if (this == &other) {
                return *this;
            }
            this->key = strdup(other.key);
            memcpy(this->value, other.value, sizeof(this->value));
            return *this;
        }
        ~KeyValue() {
            if (this->key) {
                free((void*)this->key);
                this->key = nullptr;
            }
        }
    };
    static_assert(sizeof(KeyValue) == sizeof(key_value), "doesn't match");

    void addModule(module_function_t function, module_store_t store = MODULE_STORE_MAIN, bool present = true);
    void addModule(hal_module_t module, bool present = true);
    void addModuleThatShouldNotBePresentInJson(module_function_t function, module_store_t store = MODULE_STORE_MAIN);
    void addModuleThatShouldNotBePresentInJson(hal_module_t module);
    void setPlatformId(int platformId);
    void addKeyValue(const std::string& key, const std::string& value);
    void addKeyValue();
    void reset();

    std::vector<hal_module_t> getOnlyPresentModules() const {
        std::vector<hal_module_t> out;
        auto predicate = [](const hal_module_t& mod) {
            return mod.bounds.mcu_identifier && particle::system::is_module_function_valid((module_function_t)mod.info.module_function);
        };
        std::copy_if(moduleStore_.begin(), moduleStore_.end(), std::back_inserter(out), predicate);
        return out;
    }
    std::vector<KeyValue> getKeyValues() const {
        return keyValues_;
    }

    unsigned getPlatformId() const {
        return platformId_;
    }

private:
    MockRepository* mocks_;

    unsigned platformId_;

    int info_get(hal_system_info_t* info, uint32_t flags, void* reserved);
    int info_free(hal_system_info_t* info, void* reserved);

    std::vector<hal_module_t> moduleStore_;
    std::vector<KeyValue> keyValues_;
};

} // namespace test

} // namespace particle
